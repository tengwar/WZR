
import java.util.*;
import java.io.*;

import jade.lang.acl.*;
import jade.content.*;
import jade.content.onto.basic.*;
import jade.content.lang.*;
import jade.content.lang.sl.*;
import jade.core.*;
import jade.core.behaviours.*;
import jade.domain.*;
import jade.domain.mobility.*;
import jade.domain.JADEAgentManagement.*;
import jade.gui.*;


public class ControllerAgent extends GuiAgent {
// --------------------------------------------

   private jade.wrapper.AgentContainer home;
   private jade.wrapper.AgentContainer[] container = null;
   private Map locations = new HashMap();
   private Vector agents = new Vector();
   private int agentCnt = 0;
   private int command;
   transient protected ControllerAgentGui myGui;

   public static final int QUIT = 0;
   public static final int NEW_AGENT = 1;
   public static final int MOVE_AGENT = 2;
   public static final int CLONE_AGENT = 3;
   public static final int KILL_AGENT = 4;

   // Get a JADE Runtime instance
   jade.core.Runtime runtime = jade.core.Runtime.instance();

   StanInagroda stany_agentow[] = new StanInagroda[100];
   
  
   int liczba_agentow = 0;                  // liczba agentow mobilnych
   Srodowisko srodowisko = new Srodowisko();
   ACLMessage list_o_stanie_agenta = new ACLMessage(ACLMessage.INFORM);

   protected void setup() {
      for (int j=0;j<100;j++)
          stany_agentow[j] = new StanInagroda();
      // Register language and ontology
      getContentManager().registerLanguage(new SLCodec());
      getContentManager().registerOntology(MobilityOntology.getInstance());

      try {
         // Create the container objects
         home = runtime.createAgentContainer(new ProfileImpl());
         container = new jade.wrapper.AgentContainer[3];
         for (int i = 0; i < 3; i++){
            container[i] = runtime.createAgentContainer(new ProfileImpl());
	 }
	 doWait(2015);

	 // Get available locations with AMS
	 sendRequest(new Action(getAMS(), new QueryPlatformLocationsAction()));

         //Receive response from AMS
         MessageTemplate mt = MessageTemplate.and(
			                  MessageTemplate.MatchSender(getAMS()),
			                  MessageTemplate.MatchPerformative(ACLMessage.INFORM));
         ACLMessage resp = blockingReceive(mt);
         ContentElement ce = getContentManager().extractContent(resp);
         Result result = (Result) ce;
         jade.util.leap.Iterator it = result.getItems().iterator();
         while (it.hasNext()) {
            Location loc = (Location)it.next();
            locations.put(loc.getName(), loc);
            System.out.println("kontener: nazwa = " + loc.getName() + " ID = " + loc.getID() + " adres = " + loc.getAddress());
         }
      }
      catch (Exception e) { e.printStackTrace(); }

      // odbiór informacji o akcji agenta, wykonanie akcji za pomocą środowiska 
      // oraz rozesłanie nowego stanu agenta wszystkim agentom
      addBehaviour(new CyclicBehaviour(this)
	{
	    public void action() {
                MessageTemplate mt = MessageTemplate.MatchConversationId("Info_o_akcji");
		ACLMessage msg= receive(mt);
		if (msg!=null)
                {
                    //int akcja = Integer.parseInt(msg.getContent());
                    byte b[] = msg.getByteSequenceContent();
                    int akcja = b[0];
                    int numer_agenta = b[1];


		    //System.out.println("Otrzymałem od agenta nr "+numer_agenta+" ("+  msg.getSender().getName()+") numer akcji = "+akcja);
                    if (akcja == -1)  // agent chce by wylosować mu stan początkowy
                    {
                        stany_agentow[liczba_agentow] = srodowisko.losuj_stan_pocz(stany_agentow);
                        System.out.println("wylosowalem agentowi "+liczba_agentow+"stan pocz = ("+
                                stany_agentow[liczba_agentow].w+","+stany_agentow[liczba_agentow].k+")");
                        StanInagroda stan = stany_agentow[liczba_agentow];
                        stan.nagroda = 100;
                        stan.nr_agenta = liczba_agentow;
                                              // numer agenta
                        //list_o_stanie_agenta.setByteSequenceContent(b);
                        try{
                            list_o_stanie_agenta.setContentObject(stan);
                        }
                        catch (Exception e) {
                            System.out.println( "Nie udalo sie zserializowac! "+e);
                            e.printStackTrace();
                        }
                        list_o_stanie_agenta.addReceiver(msg.getSender());
                        list_o_stanie_agenta.setConversationId("Info_o_stanie");
                        send(list_o_stanie_agenta);

                        liczba_agentow ++;
                        System.out.println("Controller: nowy agent nr "+(liczba_agentow-1)+ " liczba ag. = "+liczba_agentow);
                    }
                    else              // agent chce uzyskać stan oraz nagrodę po wykonaniu akcji
                    {
                        // uzyskuję stan i nagrodę od środowiska na podstawie obecnego stanu agenta,
                        // jego akcji oraz stanów wszystkich agentów:
                        //System.out.println("pytam srodow. czy agent "+numer_agenta+ " o stanie ("+
                        //        stany_agentow[numer_agenta].w+","+stany_agentow[numer_agenta].k+") moze wykonac akcję "+akcja);

                        stany_agentow[numer_agenta] = srodowisko.ruch_agenta(stany_agentow[numer_agenta],akcja,stany_agentow, liczba_agentow);
                        stany_agentow[numer_agenta].nr_agenta = numer_agenta;

                        //System.out.println("nowy stan agenta "+numer_agenta+" to ("+
                        //        stany_agentow[numer_agenta].w+","+stany_agentow[numer_agenta].k+")");
                        try{
                            list_o_stanie_agenta.setContentObject(stany_agentow[numer_agenta]);
                        }
                        catch (Exception e) {
                            System.out.println( "Nie udalo sie zserializowac! "+e);
                            e.printStackTrace();
                        }

                        list_o_stanie_agenta.setConversationId("Info_o_stanie");
                        send(list_o_stanie_agenta);
                    }

                }
		block();
	    }
	});

        // wysłanie agentowi mobilnemu informacji o jego własnym stanie
        addBehaviour(new CyclicBehaviour(this)
        {
	    public void action() {
                MessageTemplate mt = MessageTemplate.MatchConversationId("Zapytanie_o_stan");
		ACLMessage msg = receive(mt);
		if (msg!=null)
                {
                    int numer_agenta = Integer.parseInt(msg.getContent());
                    ACLMessage reply = msg.createReply();
                    stany_agentow[numer_agenta].nr_agenta = numer_agenta;
                    try{
                        reply.setContentObject(stany_agentow[numer_agenta]);
                    }
                    catch (Exception e) {
                        System.out.println( "Nie udalo sie zserializowac! "+e);
                        e.printStackTrace();
                    }

                    reply.setConversationId("Info_o_stanie");
                    send(reply);
                    System.out.println("Controller: jeszcze raz wysłałem agentowi "+numer_agenta+" jego stan ");
                }
		block();
	    }
	});
        // Create and show the gui
        myGui = new ControllerAgentGui(this, locations.keySet());
        myGui.setVisible(true);
   }


   protected void onGuiEvent(GuiEvent ev) {
   // ----------------- Wysyłanie rozkazów do agentów mobilnych

      command = ev.getType();

      if (command == QUIT) {
         try {
            home.kill();
            for (int i = 0; i < container.length; i++) container[i].kill();
         }
         catch (Exception e) { e.printStackTrace(); }
         myGui.setVisible(false);
         myGui.dispose();
         doDelete();
         System.exit(0);
      }
      if (command == NEW_AGENT) {
         jade.wrapper.AgentController a = null;
         try {
            Object[] args = new Object[2];
            args[0] = getAID();
            String name = "Agent"+agentCnt++;
            a = home.createNewAgent(name, MobileAgent.class.getName(), args);
            a.start();
            agents.add(name);
            myGui.updateList(agents);
         }
         catch (Exception ex) {
            System.out.println("Controller: Problem creating new agent");
         }
         return;
      }
      String agentName = (String)ev.getParameter(0);
      AID aid = new AID(agentName, AID.ISLOCALNAME);

      if (command == MOVE_AGENT) {
         //String destName = "C";
         String destName = (String)ev.getParameter(1);
         Location dest = (Location)locations.get(destName);
         MobileAgentDescription mad = new MobileAgentDescription();
         mad.setName(aid);
         mad.setDestination(dest);
         MoveAction ma = new MoveAction();
         ma.setMobileAgentDescription(mad);
         sendRequest(new Action(aid, ma));
      }
      else if (command == CLONE_AGENT) {

         String destName = (String)ev.getParameter(1);
         Location dest = (Location)locations.get(destName);
         MobileAgentDescription mad = new MobileAgentDescription();
         mad.setName(aid);
         mad.setDestination(dest);
         String newName = "Clone-"+agentName;
         CloneAction ca = new CloneAction();
         ca.setNewName(newName);
         ca.setMobileAgentDescription(mad);
         sendRequest(new Action(aid, ca));
         agents.add(newName);
         myGui.updateList(agents);
      }
      else if (command == KILL_AGENT) {
         KillAgent ka = new KillAgent();
         ka.setAgent(aid);
         sendRequest(new Action(aid, ka));
         agents.remove(agentName);
         myGui.updateList(agents);
      }
   }


   void sendRequest(Action action) {
   // -------------------

      ACLMessage request = new ACLMessage(ACLMessage.REQUEST);
      request.setConversationId("Request");
      request.setLanguage(new SLCodec().getName());
      request.setOntology(MobilityOntology.getInstance().getName());
      try {
         getContentManager().fillContent(request, action);
         request.addReceiver(action.getActor());
         send(request);
      }
      catch (Exception ex) { ex.printStackTrace(); }
   }

}//class Controller
