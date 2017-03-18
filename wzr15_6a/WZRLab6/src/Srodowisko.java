
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;
import java.util.Random;
import javafx.util.Pair;

/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

/**
 *
 * @author J
 */
public class Srodowisko {

   // tablica pol:
   //  0 - puste
   //  1 - przeszkoda
   //  2 - nagroda
   //  3 - kara
   //  4 - biegun

   int pola1[][] = {{0,0,1,0,0,0,0,2},
                   {0,0,1,1,0,1,1,2},
                   {0,0,0,0,0,0,1,2},
                   {0,0,1,1,1,0,1,0},
                   {0,0,1,0,0,0,0,2} };

   int pola2[][] = {{0,0,0,0,0,0,0,0,2},
                   {0,0,0,1,1,0,1,1,2},
                   {0,0,0,0,0,0,0,3,2},
                   {0,0,0,1,1,1,0,1,0},
                   {0,0,0,0,0,0,0,0,2}};

   int pola3[][] = {{0,0,0,0,0,0,0,0,0,2},
                   {0,0,0,0,1,1,1,1,1,2},
                   {0,0,0,0,0,0,0,0,0,2},
                   {0,0,0,4,1,1,1,1,1,2},
                   {0,0,0,4,0,0,0,0,0,0},
                   {0,0,0,0,1,1,1,1,1,2},
                  {0,0,0,0,0,0,0,0,0,2} };

   int pola4[][] = {{0,0,1,0,0,0,0,0,0,0,0,0,0,0,2},
                    {0,0,1,0,0,0,0,0,0,1,1,1,1,1,2},
                    {0,0,1,0,0,0,0,0,0,0,0,0,0,0,2},
                    {0,0,0,0,0,0,0,0,1,1,1,1,1,1,2},
                    {0,0,1,0,0,0,0,0,0,0,0,0,0,0,0},
                    {0,0,1,0,0,0,0,0,0,1,1,1,1,1,2},
                    {0,0,1,0,0,0,0,0,0,0,0,0,0,0,2},
                    {0,0,1,0,0,0,0,0,0,1,1,1,1,1,2},
                    {0,0,1,0,0,0,0,0,0,0,0,0,0,0,2}};

   int pola5[][] = {{0,0,0,0,0,3,0,2},
                   {0,0,4,4,0,0,0,2},
                   {0,0,3,3,0,0,0,2},
                   {0,0,0,0,0,0,0,0},
                   {0,0,0,0,0,0,0,0} };

   int pola6[][] = {{0,0,1,2,0,0,0,0,0,0,0,4,0},  // są dwie drogi do nagrody. W górnej jest niebezpieczeńswo kary, więc agent uczony w środowisku
                   {0,0,4,1,1,1,1,1,1,0,0,4,0},  // jednoagentowym zwyklę ją omija. Gdy podczas uczenia występują interakcje z innymi agentami,
                   {0,0,0,0,0,0,0,0,0,0,0,0,0},  // agenty uczone powinny podzielić się na dwie grupy - dolną i górną, jako że kolizje z innymi
                   {0,0,0,0,0,0,1,0,1,1,0,3,0},  // agentami też kosztują
                   {0,0,0,0,0,0,0,0,0,0,0,0,0},
                   {0,0,0,1,1,1,1,1,1,0,0,0,0},
                   {0,0,1,2,0,0,0,0,0,0,0,0,0}};
   
   int pola7[][] = {{0,0,1,0,0,0,4,1,0,2},  // którą drogę (dolną czy górną) wybierze agent w zal. od stopnia losowości (proc_wbok, proc_wtyl)?
                   {0,0,1,0,1,0,4,1,0,2},
                   {0,0,1,0,1,0,0,1,0,0},
                   {0,0,1,0,1,0,0,0,0,0},
                   {0,0,0,0,1,4,4,0,1,0},
                   {0,0,1,0,3,3,3,0,3,2},
                   {0,0,0,0,0,0,0,0,0,2} };

   int pola8[][] = {{0,0,0,0,0,0,0,0,0,3,3,0,0,0,2},
                   {0,3,0,0,4,1,0,0,0,0,0,0,0,3,2},
                   {0,0,0,0,4,1,0,0,0,1,0,0,0,0,2},
                   {0,0,0,0,1,0,0,0,0,0,1,0,0,0,2},
                   {0,0,1,0,1,0,1,1,0,0,0,0,1,0,2},
                   {0,0,1,0,0,0,0,0,0,0,0,1,0,0,2},
                   {0,0,0,0,1,1,1,3,0,0,0,3,0,0,0},
                   {0,0,0,0,0,0,0,0,0,3,0,0,0,0,0},
                   {0,0,3,3,0,1,0,0,0,1,1,1,0,4,0},
                   {0,0,4,4,0,1,0,0,0,0,2,0,0,4,0}};
   
   int pola9[][] = {{0,1,0,1,1,0,0,0,0,0,0,0,0,0},               // pole pierścieni
                   {0,0,0,0,0,0,0,0,0,0,0,0,0,0},
                   {0,0,1,0,0,0,1,1,1,0,0,0,0,0},
                   {0,0,1,0,0,4,4,4,4,4,0,3,0,0},
                   {0,0,1,0,0,4,4,4,4,4,0,0,0,2},
                   {0,0,1,0,0,4,4,4,4,4,0,0,0,2},
                   {0,1,1,0,0,4,4,4,4,4,0,0,0,2},
                   {0,0,0,0,0,4,4,4,4,4,0,0,0,0},
                   {0,0,0,0,0,0,0,0,0,0,0,0,0,0},
                   {0,0,0,0,0,0,3,3,0,0,0,0,0,0}};

    int pola10[][] = {
        {0,0,0,0,0,3,0,0,0,0,0,0,0,1,0,0,0,0,0,0,3,3,0,0,0,0,0}, // pierścienie w labiryncie
        {0,0,0,0,3,0,0,0,0,1,1,1,1,1,0,0,1,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1,1,1,0,0,0,0},
        {0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,3,1,0,0,0,0,0,0},
        {0,0,0,1,0,0,1,1,1,0,0,0,0,0,1,1,1,0,0,3,1,0,1,0,0,0,2},
        {0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,0,0,2},
        {0,0,0,0,0,1,0,0,0,0,0,1,1,1,1,0,0,0,1,1,1,0,1,0,0,0,2},
        {0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0},
        {0,0,0,0,3,0,0,1,1,0,0,1,1,1,0,0,0,0,0,1,1,1,0,0,0,0,0},
        {0,0,0,0,3,0,0,1,0,0,1,0,0,0,0,1,1,0,1,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,1,0,0,1,0,0,0,1,4,4,0,1,3,0,1,0,0,1,0,0},
        {0,0,0,0,0,0,0,0,0,0,1,0,0,1,0,4,4,0,0,1,0,3,3,3,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,1,0,0,1,1,1,1,1,1,1,0,0,3,3,1,0,0},
        {0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,3,3,0,0,0}};

   int pola11[][] = {{0,0,0,0,3,3,0,0,4,4,0,2},
                     {0,0,0,3,3,3,0,0,0,0,0,2},        // dużo kar
                     {0,3,0,0,0,0,0,0,0,0,0,2},
                     {0,3,0,3,0,3,0,0,0,0,0,2},
                     {0,3,0,3,0,0,0,3,0,0,0,2},
                     {0,3,0,0,0,3,0,0,0,0,0,2}};


   int pola[][] = pola5;
  
   int liczba_wierszy = pola.length;
   int liczba_kolumn = pola[0].length;
   int proc_wbok = 10;     // prawdopodobienstwo w [%] pojscia w bok od wybranego kierunku
   int proc_wtyl = 5;      // -||-  pojscia w tyl od wybranego kierunku
   int proc_wmiejscu = 0;  // -||-  pozostania w miejscu
   int wartosc_nagrody_za_zolte = 40;
   int wartosc_za_zamkniecie = 300;  // wartość nagrody za zamknięcie obwodu
   int wartosc_kary_za_ruch = -1;
   int wartosc_kary_za_kolizje_ze_sciana = -4;
   int wartosc_kary_za_kolizje_z_agentem = -16;
   int wartosc_kary = -60;    // kara za wjechanie na czerwone pole
   boolean dopuszcz_brak_ruchu = true;   // czy można wybrać akcję zostania w miejscu
   int proc_ruch = 10;                   // prawdopodobieństwo wykonania ruchu, jeśli agent chce zostać w miejscu

   

   // Funkcja modelujaca ruch zbioru obiektow nalezacych do pewnego agenta
   // w srodowisku: na podstawie
   // aktualnego stanu, akcji agenta, stanów wszystkich agentów (w tym naszego) i
   // liczby wszystkich agentów srodowisko zwraca
   // nowy stan w postaci nowych polozen obiektow i nagrod: 
   // {{x1,y1,r1},{x2,y2,r2},...,{xn,yn,rn}}
   // na wejscu: akcja w postaci tablicy posuniec obiektow:
   // numery akcji: 0 - brak ruchu, 1 - w prawo, 2 - w dół, 3 - w lewo, 4 - do góry,
   StanInagroda ruch_agenta(StanInagroda stan, int akcja, StanInagroda stany_agentow[], int liczba_agentow)
   {
       StanInagroda nowy_stan_i_nagroda = new StanInagroda();
       nowy_stan_i_nagroda.nagroda = 0;

       Random loteria=new Random();
      
       //int nowy_stan[] = new int[2];
     
       // losowanie przejścia docelowego:
       // (agent moze pojsc przypadkowo w innym kierunku niz zamierzal)
       int akcja_rzecz = akcja;
       int proc_los = loteria.nextInt(100);
       if (akcja == 0)
       {
           if ((proc_los < proc_ruch)||(dopuszcz_brak_ruchu == false))
               akcja_rzecz = 1+loteria.nextInt(4);
       }
       else
       {
           if (proc_los < proc_wtyl)
           {
               if (akcja==1) akcja_rzecz = 3;
               else if (akcja==2) akcja_rzecz = 4;
               else if (akcja==3) akcja_rzecz = 1;
               else if (akcja==4) akcja_rzecz = 2;
           }
           else if (proc_los < proc_wbok+proc_wtyl)
           {
               int lewo = loteria.nextInt(2);
               if (akcja==1) akcja_rzecz = (lewo == 0 ? 2 : 4);
               else if (akcja==2) akcja_rzecz = (lewo == 0 ? 3 : 1);
               else if (akcja==3) akcja_rzecz = (lewo == 0 ? 4 : 2);
               else if (akcja==4) akcja_rzecz = (lewo == 0 ? 1 : 3);
           }
           else if ((proc_los < proc_wbok+proc_wtyl+proc_wmiejscu)&&(dopuszcz_brak_ruchu))
           {
               akcja_rzecz = 0;
           }
       }

       // wykonanie akcji -> przejscie do nowego stanu
       int wiersz = stan.w, kolumna = stan.k;

       if (akcja_rzecz == 1) kolumna++;                 // w prawo
       else if (akcja_rzecz == 2) wiersz++;             // do dołu
       else if (akcja_rzecz == 3) kolumna--;            // w lewo
       else if (akcja_rzecz == 4) wiersz--;             // w górę
       if ((wiersz >=0)&&(wiersz < liczba_wierszy)&&    // sprawdzenie czy nie wychodzi poza bande
           (kolumna >=0)&&(kolumna < liczba_kolumn)&&
           (pola[wiersz][kolumna] != 1))                // sprawdzenie czy w polu nie ma sciany
       {
         nowy_stan_i_nagroda.w = wiersz;
         nowy_stan_i_nagroda.k = kolumna;
       }
       else            // cofnięcie do stanu poprzedniego
       {
         nowy_stan_i_nagroda.w = stan.w;
         nowy_stan_i_nagroda.k = stan.k;
         nowy_stan_i_nagroda.nagroda += wartosc_kary_za_kolizje_ze_sciana;
       }

       // sprawdzenie czy nie ma kolizji z innym agentem:
       for (int i=0;i<liczba_agentow;i++)
           if ((stany_agentow[i].w != stan.w)||(stany_agentow[i].k != stan.k)) // jeśli nie jest to nasz agent
           {
               if ((stany_agentow[i].w == nowy_stan_i_nagroda.w)&&(stany_agentow[i].k == nowy_stan_i_nagroda.k)) // jeśli kolizja
               {
                   nowy_stan_i_nagroda.w = stan.w;           // cofnięcie do stanu poprzedniego
                   nowy_stan_i_nagroda.k = stan.k;
                   nowy_stan_i_nagroda.nagroda += wartosc_kary_za_kolizje_z_agentem;
                   break;
               }
           }
     

       // nagroda lub kara za wykonana akcje oraz przejscie do nowego stanu:
       if (pola[nowy_stan_i_nagroda.w][nowy_stan_i_nagroda.k] == 2)
           nowy_stan_i_nagroda.nagroda += wartosc_nagrody_za_zolte;
       else if (pola[nowy_stan_i_nagroda.w][nowy_stan_i_nagroda.k] == 3)
           nowy_stan_i_nagroda.nagroda += wartosc_kary;

       if ((nowy_stan_i_nagroda.w != stan.w)||(nowy_stan_i_nagroda.k != stan.k))
           nowy_stan_i_nagroda.nagroda += wartosc_kary_za_ruch;

       // nagroda za zamknięcie obwodu (jeśli dwóch agentów zajmie dwa sąsiednie bieguny):
       if (pola[nowy_stan_i_nagroda.w][nowy_stan_i_nagroda.k] == 4)
       {
           for (int i=0;i<liczba_agentow;i++)
               if (((stany_agentow[i].w != stan.w)||(stany_agentow[i].k != stan.k))&& // jeśli nie jest to nasz agent
               (pola[stany_agentow[i].w][stany_agentow[i].k] == 4)&&                    // na sąsiednim polu z pierścienim stoi inny agent
               (Math.abs(stany_agentow[i].w-nowy_stan_i_nagroda.w)+Math.abs(stany_agentow[i].k-nowy_stan_i_nagroda.k) == 1)) // jest to pole sąsiednie
               {
                   nowy_stan_i_nagroda.nagroda += wartosc_za_zamkniecie;
                   //System.out.println("nagroda za zamknięcie dzięki agentowi "+i+" = "+stan_i_nagroda[2]);
               }
       }

       // sprawdzenie czy obiekt agenta nie znajduje się w ostatniej kolumnie lub zdobył nagrodę. 
       // Jeśli tak, to przejście do pierwszej kolumny
       if ((nowy_stan_i_nagroda.k >= liczba_kolumn-1)||(nowy_stan_i_nagroda.nagroda > 0))
       {
           nowy_stan_i_nagroda.k = 0;                                // przejscie na poczatek planszy
           nowy_stan_i_nagroda.w = loteria.nextInt(liczba_wierszy);  // do losowego wiersza
       }

       return nowy_stan_i_nagroda;
   }

   StanInagroda losowanie_stanu_pocz(StanInagroda stany_innych[], int liczba_innych)
   {
        // wybor stanow poczatkowych obiektow (tak by nie kolidowaly z innymi obiektami
        // agenta naszego i obcych agentow:
        int puste_pola[] = new int[liczba_wierszy];
        int liczba_pustych_pol = 0;
        int agenty_w_kol0[] = new int[liczba_wierszy];
        
        for (int i=0;i<liczba_wierszy;i++) agenty_w_kol0[i] = 0;   // wyzerowanie tablicy zajętości pól pierwszej kolumny
        
        for (int i=0;i<liczba_innych;i++) 
            if (stany_innych[i].k == 0) agenty_w_kol0[stany_innych[i].w] = i;   // umieszczenie numeru agenta znajdujacego się w pierwszej kolumnie
        
        for (int i=0;i<liczba_wierszy;i++)                         // utworzenie tablicy pustych pól w pierwszej kolumnie
            if ((pola[i][0] != 1)&&(agenty_w_kol0[i] == 0))        // jeśli nie ma w polu i przeszkody ani innego agenta         
            {
              puste_pola[liczba_pustych_pol] = i;
              liczba_pustych_pol++;
            }

        StanInagroda stan = new StanInagroda();
        if (liczba_pustych_pol == 0) {   // nie da się umieścic agenta  
            stan.w = -1;
            stan.k = -1;
        }
        else
        {
            Random loteria = new Random();
            stan.w = loteria.nextInt(liczba_pustych_pol);
            stan.k = 0;
        }

        return stan;
   }
   
    StanInagroda losuj_stan_pocz(StanInagroda stany_innych[])
    {
        List<Pair<Integer, Integer>> puste_pola = new ArrayList<>();
        List<Pair<Integer, Integer>> pola_zajęte_przez_agentow = new ArrayList<>();
        
        // dodaj pola bez przeszkód do listy pustych
        for (int w = 0; w < liczba_wierszy; w++) {
            for (int k = 0; k < liczba_kolumn; k++) {
                if (pola[w][k] != 1) {
                    puste_pola.add(new Pair<>(w, k));
                }
            }
        }
        
        // zrób listę pól zajętych przez agentów
        for (StanInagroda stan : stany_innych) {
            pola_zajęte_przez_agentow.add(new Pair<>(stan.w, stan.k));
        }
        
        // usuń pola zajęte prze agentów z listy pustych pól
        puste_pola.removeAll(pola_zajęte_przez_agentow);
        
        // losuj element z listy pustych
        Random rand = new Random();
        int element_index = rand.nextInt(puste_pola.size());
        Pair<Integer, Integer> polozenie = puste_pola.get(element_index);
        
        // stwórz obiekt do zwrócenia
        StanInagroda stan = new StanInagroda();
        stan.w = polozenie.getKey();    // w
        stan.k = polozenie.getValue();  // k
        
        return stan;
    }


   // Ocena  strategii dla agenta o podanym numerze nr_agenta w interakcji z innymi agentami w sposób
   // asynchroniczny: wszystkie agenty wykonują posunięcia w sposób asynchroniczny - bez ustalonej kolejności
   double ocen_strategie(Strategia[] strategie_wszystkie, int nr_agenta, int liczba_agentow, int liczba_epizodow, int liczba_krokow_max)
   {
       
      //int stany[][] = new int[liczba_agentow][2];   // stan agenta (numer wiersza i kolumny)
      StanInagroda stany[] = new StanInagroda[liczba_agentow];
      for (int j=0;j<liczba_agentow;j++)
          stany[j] = new StanInagroda();
      StanInagroda stan_i_nagroda = new StanInagroda();

      Random loteria = new Random();
      
      float suma_nagrod = 0;
      for (int i = 0;i<liczba_epizodow;i++)
      {
          // losowanie stanu początkowego -> położenie agenta w pierwszej kolumnnie
          for (int j=0;j<liczba_agentow;j++){
            stany[j].w = loteria.nextInt(liczba_wierszy);
            stany[j].k = 0;
          }
          int nr_kroku = 0;

          while (nr_kroku < liczba_krokow_max)
          {
              int nr_ag = loteria.nextInt(liczba_agentow);   // lepiej losować czasy opóźnień

              int akcja = strategie_wszystkie[nr_ag].nry_akcji[0][stany[nr_ag].w][stany[nr_ag].k];

              //int[] stan_i_nagroda = s.ruch_agenta(stan,akcja,stany_obcych_agentow,liczba_obcych_agentow);
              stan_i_nagroda = ruch_agenta(stany[nr_ag],akcja,stany,liczba_agentow);

              stany[nr_ag].w = stan_i_nagroda.w;
              stany[nr_ag].k = stan_i_nagroda.k;
              if (nr_ag == nr_agenta){
                suma_nagrod += stan_i_nagroda.nagroda;
                nr_kroku ++;
              }
          }
      }
      return  suma_nagrod/liczba_epizodow;
   }  // koniec funkcji oceny strategii


   // ocena strategii agenta w oparciu tylko o interakcje ze środowiskiem (bez uwzględnienia obecności innych agentow)
   double ocen_strategie(int[][] strategia, int liczba_epizodow, int liczba_krokow_max)
   {
      StanInagroda stan = new StanInagroda();   // stan agenta (numer wiersza i kolumny)
      int liczba_innych_agentow = 0;
      StanInagroda polozenia_innych[] = new StanInagroda[liczba_innych_agentow]; // poĹ‚oĹĽenia innych agentĂłw
     

      Random loteria = new Random();

      float suma_nagrod = 0;
      for (int i = 0;i<liczba_epizodow;i++)
      {
          // losowanie stanu poczÄ…tkowego -> poĹ‚oĹĽenie agenta w pierwszej kolumnnie
          stan.w = loteria.nextInt(liczba_wierszy);
          stan.k = 0;
          int nr_kroku = 0;

          while ((stan.k != liczba_kolumn-1)&&(nr_kroku < liczba_krokow_max))
          {
              int akcja = strategia[stan.w][stan.k];

              //int[] stan_i_nagroda = s.ruch_agenta(stan,akcja,stany_obcych_agentow,liczba_obcych_agentow);
              StanInagroda stan_i_nagroda = ruch_agenta(stan,akcja,polozenia_innych,0);

              stan.w = stan_i_nagroda.w;
              stan.k = stan_i_nagroda.k;
              suma_nagrod += stan_i_nagroda.nagroda;

              nr_kroku ++;
          }
      }
      return  suma_nagrod/liczba_epizodow;
   }   // koniec funkcji oceny strategii
}
