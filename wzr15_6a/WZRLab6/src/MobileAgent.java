

import jade.core.*;
import jade.core.behaviours.*;
import jade.lang.acl.*;
import jade.content.*;
import jade.content.lang.sl.*;
import jade.content.onto.basic.*;
import jade.domain.*;
import jade.domain.FIPAAgentManagement.AMSAgentDescription;
import jade.domain.FIPAAgentManagement.SearchConstraints;
import jade.domain.mobility.*;
import jade.domain.JADEAgentManagement.*;
import jade.gui.*;
import jade.util.leap.Serializable;

// położenie agenta bez uwzględnienia zmienności środowiska i innych agentów
class StanInagroda implements Serializable {
    int w,k;                   // numer wiersza i kolumny
    int nagroda;
    int nr_agenta;
}

interface TypyStrategii {
    int PROSTA = 1, WYKR_SASIADOW = 2;
} 

class Strategia implements Serializable {
    int liczba_plansz = 1;                   // liczba plansz (w najprostszym przypadku 1) lub inaczej liczba warości innych parametrów
                                             // niż plansza np. żeby nauczyć się unikania kolizji z innymi agentami, możemy dodać informację czy sąsiednie
                                             // pola są wolne. Takich możliwości jest 2^4 == 16, stąd n = 16, czyli potrzebujemy 16 plansz kierunków ruchu,
                                             // każda dla każdej kombinacji wolnych pól
    int liczba_wierszy,liczba_kolumn;
    int nr_agenta;                           // numer agenta o tej strategii
    int nry_akcji[][][];                     // numery akcji po n, wierszach i kolumnach

    Strategia(int typ_strategii, int l_wierszy, int l_kolumn)
    {
        liczba_wierszy = l_wierszy;
        liczba_kolumn = l_kolumn;
        switch (typ_strategii){
            case TypyStrategii.PROSTA:
                liczba_plansz = 1;
                break;
            case TypyStrategii.WYKR_SASIADOW:
                liczba_plansz = 16;
                break;
        }
        
        nry_akcji = new int[liczba_plansz][liczba_wierszy][liczba_kolumn];
    }


}


public class MobileAgent extends GuiAgent {
// ----------------------------------------
   int dlugosc_pauzy = 100;  // długość przerwy w animacji w [ms]
   long maks_liczba_epizodow = 1000000;
   int typ_strategii = TypyStrategii.PROSTA;

   int UCZENIE_ZE_WZMOCNIENIEM = 0, ALGORYTM_GENETYCZNY = 1; 
   int met_uczenia = UCZENIE_ZE_WZMOCNIENIEM;
   
   private AID controller;
   private Location destination;
   transient protected MobileAgentGui myGui;
   MobileAgent me = this;
   int numer_stanu = 1;
   AMSAgentDescription [] agents = null;

   Srodowisko srodowisko = new Srodowisko();

   //int moj_stan[] = new int[2];   // stan agenta (numer wiersza i kolumny)
   StanInagroda moj_stan = new StanInagroda();


   int moja_akcja = -1;
   float moje_konto = 0;

   StanInagroda stany_agentow[] = new StanInagroda[100]; // stany wszystkich agentow
   int liczba_agentow = 1;                               // liczba wszystkich agentow
   int moj_numer = -1;                                   // numer tego agenta (od zera)
   boolean czekam_na_wyk_akcji = false;  // żeby nie wykonać następnej akcji, zanim się nie dowie gdzie się jest
   long czas = System.currentTimeMillis(),czas_start;

   ACLMessage msg_o_strat = new ACLMessage(ACLMessage.INFORM); // list informujący innych agentów o własnej strategii

   AlgGenetyczny ag = new AlgGenetyczny();
   UczenieZeWzm uzw = new UczenieZeWzm(typ_strategii);
   Watek_AG watek_ag = new Watek_AG(); 
   
   Strategia strategia = uzw.strategia;                             // aktualna strategia tego agenta
   Strategia strategie_wszystkie[] = new Strategia[100];            // strategie innych agentów
        //watek_ag.run();   
   
   protected void setup() {
// ------------------------

       for (int j=0;j<100;j++){
          stany_agentow[j] = new StanInagroda();
          strategie_wszystkie[j] = new Strategia(typ_strategii,srodowisko.liczba_wierszy,srodowisko.liczba_kolumn);
       }
       
        // Retrieve arguments passed during this agent creation
        Object[] args = getArguments();
        controller = (AID) args[0];
        destination = here();
        
        init();

        //Random loteria=new Random();

        // Zapytanie o stan początkowy i o numer ID:
        ACLMessage msg = new ACLMessage(ACLMessage.INFORM);
        byte b[] = new byte[2];
        b[0] = -1;
        b[1] = -1;                                     // numer agenta
        msg.setByteSequenceContent(b);
        //msg.setContent(String.valueOf(-1));      // wysyłam specjalną akcję o numerze -1, by agent kontroler wylosował stan początkowy
        msg.setConversationId("Info_o_akcji");
        msg.addReceiver(controller);
        send(msg);
        // odbiór informacji o stanie początkowym oraz o przydzielonym numerze identyfikacyjnym
        MessageTemplate mt = MessageTemplate.MatchConversationId("Info_o_stanie");
	ACLMessage msg2= blockingReceive(mt);
	if (msg2!=null)
        {
            try{
                moj_stan = (StanInagroda)msg2.getContentObject();
                moj_numer = moj_stan.nr_agenta;
                uzw.moj_numer = moj_numer;
                ag.moj_numer = moj_numer;

                System.out.println("moj_numer = "+moj_numer+" nagroda = "+moj_stan.nagroda);

            }
            catch (Exception e) {
                System.out.println( "Nie udalo sie odserializowac! "+e);
                e.printStackTrace();
            }
        }
        liczba_agentow = moj_numer+1;

        // kopiowanie wartości i wskazników do tablic:
        //myGui.UstawRozmiaryPlanszy(liczba_wierszy, liczba_kolumn);
        myGui.plansza.moj_numer = moj_numer;
        myGui.plansza.liczba_kolumn = srodowisko.liczba_kolumn;
        myGui.plansza.liczba_wierszy = srodowisko.liczba_wierszy;
        myGui.plansza.pola = srodowisko.pola;
        myGui.plansza.moj_stan = moj_stan;
        myGui.plansza.stany_agentow = stany_agentow;

        // Program the main behaviour of this agent
        addBehaviour(new ReceiveCommands(this));

      
        // Wyszukiwanie agentow na platformie:
        try {
            SearchConstraints c = new SearchConstraints();
            c.setMaxResults (new Long(-1));
            agents = AMSService.search(me, new AMSAgentDescription (), c );
            for (int i=0; i<agents.length;i++)
            {
                //System.out.println("Jestem " + getAID().getName() + " AMS znalazl agenta " + agents[i].getName() );
            }
        }
        catch (Exception e) {
            System.out.println( "Problem  searching AMS: " + e );
            e.printStackTrace();
        }

        // Wyslanie listu do wszystkich istniejacych agentow:
        ACLMessage msg3 = new ACLMessage(ACLMessage.INFORM);
        //msg3.setContent( "Witam! Moj stan = " + numer_stanu );
        msg3.setContent( "Witam! Jestem agent " + this.getLocalName() +  " z kontenera " + destination.getName());
        msg3.setConversationId("Info_o_utworzeniu");
        String moje_imie = getAID().getName();
        for (int i=0; i<agents.length;i++)
            if ( moje_imie.equals(agents[i].getName().getName()) == false) // by sobie samemu nie wysylac
            {
                msg3.addReceiver( agents[i].getName() );
                msg_o_strat.addReceiver( agents[i].getName());
            }
        send(msg3);
        
        msg_o_strat.setConversationId("Info_o_strategii");



        addBehaviour(new CyclicBehaviour(this) // oczekiwanie na wiadomosc o utworzeniu innego agenta
	{
	    public void action() {

                numer_stanu++;
                MessageTemplate mt = MessageTemplate.MatchConversationId("Info_o_utworzeniu");
		ACLMessage msg= receive(mt);
		if (msg!=null)
                {
		    //System.out.println("Jestem " + getAID().getName()+"("+numer_stanu+")" + ", dostalem list: \""
			//		 +  msg.getContent() + "\" od "
			//		 +  msg.getSender().getName() );
                    msg_o_strat.addReceiver(msg.getSender());  // zapisuje adres agenta, by późnej wysyłać mu informacje o strategii
                }
		block();
	    }
	});

        addBehaviour(new CyclicBehaviour(this) // odbiór strategii innego agenta
        {
	    public void action() {
                //MessageTemplate mt = MessageTemplate.MatchConversationId("Info_o_strategii");
                MessageTemplate mt = MessageTemplate.and(
                        MessageTemplate.MatchConversationId("Info_o_strategii"),
                        MessageTemplate.MatchPerformative(ACLMessage.INFORM));
                ACLMessage msg= receive(mt);
                if (msg!=null)
                {
                    Strategia strategia;
                    int nr_agenta;
                    try{
                        strategia = (Strategia)msg.getContentObject();
                        nr_agenta = strategia.nr_agenta;
                        strategie_wszystkie[nr_agenta] = strategia;
                    }
                    catch (Exception e) {
                        System.out.println( "Nie udalo sie odserializowac! "+e);
                        e.printStackTrace();
                    }
                }
                else
                    block();
	    }
	});

        

        addBehaviour(new CyclicBehaviour(this)  // realizacja epoki uczenia genetycznego lub epizodu uczenia ze wzmocnieniem
        {
	    public void action() {

                if (met_uczenia == UCZENIE_ZE_WZMOCNIENIEM)
                {
                    if (uzw.numer_epizodu < maks_liczba_epizodow)
                    {

                        if (System.currentTimeMillis()-czas < 2015)
                        {
                            czas = System.currentTimeMillis();

                            uzw.strategie_wszystkie = strategie_wszystkie;
                            uzw.tworz_strategie();
                            strategie_wszystkie[moj_numer] = uzw.strategia;

                            double ocena = srodowisko.ocen_strategie(strategie_wszystkie,moj_numer,
                                    liczba_agentow, ag.liczba_epizodow_oceny ,uzw.liczba_krokow_max);
                            //System.out.printf("Śr. nagroda po %d epizodach = %f\n ",ag.liczba_epizodow_oceny,ocena);
                           
                            uzw.zapis_Q_do_pliku(moj_numer);

                            String tekst = new String("uczenie: śr nagroda po "+ag.liczba_epizodow_oceny+" epizodach = "+ocena);
                            myGui.plansza.tekst2 = tekst;

                            // wysyłam strategię do pozostałych agentów:
                            //System.out.printf("Agent %d: wysyłam strategię innym agentom po %d epizodach\n",moj_numer,uzw.numer_epizodu);

                            try{
                                uzw.strategia.nr_agenta = moj_numer;
                                msg_o_strat.setContentObject(uzw.strategia);
                            }
                            catch (Exception e) {
                                System.out.println( "Nie udalo sie odserializowac! "+e);
                                e.printStackTrace();
                            }

                            send(msg_o_strat);
                        }

                        uzw.epizod_uczenia_Qlearning();
                    }
                    else     // po zakończeniu uczenia wysyłamy co jakiś czas informację o strategii
                    {
                        String tekst = new String("koniec uczenia ze wzmocnieniem");
                        myGui.plansza.tekst2 = tekst;
                    }
                }
                else if (met_uczenia == ALGORYTM_GENETYCZNY)
                {
                    ag.strategie_wszystkie = strategie_wszystkie;
                    ag.liczba_agentow = liczba_agentow;

                    ag.epoka_uczenia();

                    strategia = ag.rozw_max.strategia;
                    strategie_wszystkie[moj_numer] = strategia;
                    if (System.currentTimeMillis()-czas < 2015)
                    {
                        czas = System.currentTimeMillis();
                        double ocena = srodowisko.ocen_strategie(strategie_wszystkie,moj_numer,
                                    liczba_agentow, ag.liczba_epizodow_oceny ,uzw.liczba_krokow_max);

                        String tekst = new String("uczenie: śr nagroda po "+ag.liczba_epizodow_oceny+" epizodach = "+ocena);
                        myGui.plansza.tekst2 = tekst;
                        try{
                            uzw.strategia.nr_agenta = moj_numer;
                            msg_o_strat.setContentObject(uzw.strategia);
                        }
                        catch (Exception e) {
                            System.out.println( "Nie udalo sie odserializowac! "+e);
                            e.printStackTrace();
                        }
                        

                        send(msg_o_strat);
                    }

                }
                    
	    }
	});


        addBehaviour(new TickerBehaviour(this,(dlugosc_pauzy*3)/2)  // wykonanie akcji wg wyuczonej strategii, a następnie wysłanie informacji kontrolerowi
        {
	    public void onTick() {    

               // losowanie akcji (0-w miejscu, 1- w lewo, 2 - do dolu, 3- w prawo, 4 -do gory
               //Random loteria=new Random();
               //if (loteria.nextInt(2) == 0) akcja = 3;
               //else akcja = 1+loteria.nextInt(4);
               //akcja = ag.rozw_max.strategia[moj_stan[0]][moj_stan[1]];
               uzw.tworz_strategie();
               moja_akcja = strategia.nry_akcji[0][moj_stan.w][moj_stan.k];

               myGui.plansza.moja_akcja = moja_akcja;
               myGui.plansza.moj_stan = moj_stan;
               myGui.plansza.repaint();
               doWait(dlugosc_pauzy);
               // zamiast tego należy wybierać akcje zgodnie ze strategią maksymalizującą zyski:
               // (lub częściowo losowo w przypadku uczenia, ale lepiej by uczenie odbywało się w oddzielnym wątku)
     

               // Ruch agenta wykonany lokalnie -> nie da się sprawdzić czy nie ma kolizji z innym agentem:
               //int[] stan_i_nagroda = srodowisko.ruch_agenta(moj_stan,akcja,stany_obcych_agentow,liczba_obcych_agentow);
               
               // Drugi sposób: wysłanie akcji kontrolerowi (agentowi zarządzającemu), a następnie oczekiwanie na wiadomość
               // o stanie i nagrodzie (kontroler w międzyczasie wykonuje akcję za pom. obiektu środowisko, biorąc
               // pod uwagę aktualne położenia innych agentów:
               // wyslanie akcji kontrolerowi:
               if (czekam_na_wyk_akcji == false)
               {
                   ACLMessage msg = new ACLMessage(ACLMessage.INFORM);
                   //msg.setContent(String.valueOf(akcja));
                   byte b[] = {(byte)moja_akcja, (byte)moj_numer};

                   //System.out.println("agent "+moj_numer+" wysyłam kontrolerowi akcje "+b[0]+" i numer "+b[1]);
                   msg.setByteSequenceContent(b);
                   msg.setConversationId("Info_o_akcji");
                   msg.addReceiver(controller);
                   czekam_na_wyk_akcji = true;
                   long czas_start = System.currentTimeMillis();
                   send(msg);
                   String tekst = new String(" w stanie (" + moj_stan.w+","+moj_stan.k+") wykonałem akcję "+
                           moja_akcja + " konto = " + moje_konto);
                   //System.out.println("Agent " + getName() + tekst);
                   myGui.plansza.tekst1 = tekst;
                   myGui.plansza.moj_stan = moj_stan;

                   //System.out.println("Agent " + getName() + " akcja = " + moja_akcja +
                   //        " wiersz = " + moj_stan[0] + " kol = " + moj_stan[1] + " konto = " + moje_konto);
               }
               else if (System.currentTimeMillis() - czas_start > 1000) // jeśli długo nie znam efektu swej akcji
               {
                   ACLMessage msg = new ACLMessage(ACLMessage.INFORM);
                   msg.setConversationId("Zapytanie_o_stan");
                   msg.setContent(""+moj_numer);
                   msg.addReceiver(controller);
                   send(msg);
                   System.out.println("agent "+moj_numer+": już od "+(System.currentTimeMillis() - czas_start)+
                           " czekam na mój stan");
               }

	    }
	});
        
        addBehaviour(new CyclicBehaviour(this) // odbiór informacji o stanie własnym lub innego agenta od kontrolera
        {
	    public void action() {
                MessageTemplate mt = MessageTemplate.MatchConversationId("Info_o_stanie");
                ACLMessage msg= receive(mt);
                if (msg!=null)
                {

                    StanInagroda stan = new StanInagroda();
                    int numer_agenta=0;
                    try{
                        stan = (StanInagroda)msg.getContentObject();
                        numer_agenta = stan.nr_agenta;
                    }
                    catch (Exception e) {
                        System.out.println( "Nie udalo sie odserializowac! "+e);
                        e.printStackTrace();
                    }

                    if (numer_agenta == moj_numer) // odebrano nasz własny stan
                    {
                        moj_stan = stan;
                        moje_konto += moj_stan.nagroda;

                        czekam_na_wyk_akcji = false;
                        moja_akcja = -1;
                        myGui.plansza.moja_akcja = moja_akcja;
                        
                        String tekst = new String(": nowy stan = "+moj_stan.w+","+moj_stan.k+" nagroda = "+
                                moj_stan.nagroda+ " konto = " + moje_konto);
                 
                        czas = System.currentTimeMillis();

                        //System.out.printf("agent %d: otrzymalem stan = (%d,%d) i nagrode %d\n",moj_numer,moj_stan.w,moj_stan.k,moj_stan.nagroda);
                    }
                    else                             // jeśli informacja o stanie innego agenta
                    {
                        stany_agentow[numer_agenta] = stan;
                        if (numer_agenta >= liczba_agentow)
                            liczba_agentow = numer_agenta+1;
                        myGui.plansza.liczba_agentow = liczba_agentow;
                        uzw.liczba_agentow = liczba_agentow;

                    }
                    myGui.plansza.repaint();
                }
                //else
                //    block();
	    }
	});
   }

   void init() {
// -------------

      // Register language and ontology
      getContentManager().registerLanguage(new SLCodec());
      getContentManager().registerOntology(MobilityOntology.getInstance());

      // Create and display the gui
      myGui = new MobileAgentGui(this);
      myGui.setVisible(true);
      myGui.setLocation(destination.getName());
   }

   protected void onGuiEvent(GuiEvent e) {
// ---------------------------------------
   //No interaction with the gui
   }

   protected void beforeMove() {
// -----------------------------

      System.out.println("Moving now to location : " + destination.getName());
      myGui.setVisible(false);
      myGui.dispose();
   }

   protected void afterMove() {
// ----------------------------

      init();
      myGui.setInfo("Arrived at location : " + destination.getName());
   }

   protected void beforeClone() {
// -----------------------------

      myGui.setInfo("Cloning myself to location : " + destination.getName());
   }

   protected void afterClone() {
// ----------------------------

      init();
   }


   /*
   * Receive all commands from the controller agent
   */
   class ReceiveCommands extends CyclicBehaviour {
// -----------------------------------------------

      ReceiveCommands(Agent a) { super(a); }

      public void action() {

         MessageTemplate mt = MessageTemplate.and(
                        MessageTemplate.MatchConversationId("Request"),
                        MessageTemplate.MatchPerformative(ACLMessage.REQUEST));
         ACLMessage msg = receive(mt);

         if (msg == null) { block(); return; }

         if (msg.getPerformative() == ACLMessage.REQUEST){

            try {
               ContentElement content = getContentManager().extractContent(msg);
               Concept concept = ((Action)content).getAction();

               if (concept instanceof CloneAction){
                  CloneAction ca = (CloneAction)concept;
                  String newName = ca.getNewName();
                  Location l = ca.getMobileAgentDescription().getDestination();
                  if (l != null) destination = l;
                  doClone(destination, newName);
               }
               else if (concept instanceof MoveAction){
                  MoveAction ma = (MoveAction)concept;
                  Location l = ma.getMobileAgentDescription().getDestination();
                  if (l != null) doMove(destination = l);

                  // na inna platforme (wersja 3.5):
                  /*AID remoteAMS = new AID("ams@192.168.1.101:1099/JADE",AID.ISGUID);
                  remoteAMS.addAddresses("http://192.168.1.101:7778/acc");
                  PlatformID dest = new PlatformID(remoteAMS);
                  doMove(dest);
                  */
                  
                  
               }
               else if (concept instanceof KillAgent){
                  myGui.setVisible(false);
                  myGui.dispose();
                  doDelete();
               }
            }
            catch (Exception ex) { ex.printStackTrace(); }
         }
         else { System.out.println("Unexpected msg from controller agent"); }
      }
   }


   class Watek_AG extends Thread
    {
       int czy_czynny = 1;

        public void run()
        {
            while (czy_czynny == 1)
            {
              ag.epoka_uczenia();
            }//koniec while
        }//koniec metody run()
    }//koniec klasy wew
} // class MobileAgent