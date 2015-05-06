import java.util.Random;
import java.lang.Math.*;
import java.io.*;

/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

/**
 *
 * @author 
 */
public class UczenieZeWzm {

    double alfa = 0.1;                // szybkosc uczenia ............................. TO NALEŻY USTAWIĆ
    double gamma = 0.97;              // wspolczynnik dyskontowania ................... TO NALEŻY USTAWIĆ
    double epsylon = 0.2;             // wspolczynnik eksploracji podczas uczenia ..... TO NALEŻY USTAWIĆ

    //boolean czy_softmax = false;      // czy eksploracja softmax (jeśli nie, to epsylon-zachłanna)
    double wsp_selekcji = 1.0;        // im większy, tym częstszy wybór akcji o większej wartości w softmax-sie ..... TO NALEŻY USTAWIĆ

    float Q[][][];                    // tablica użyteczności akcji

    Strategia strategia;              // numery akcji w każdym ze stanów (każdy stan to położenie na planszy 2D)
    Strategia strategie_wszystkie[];  // strategie innych agentów
    int liczba_wartosci = 1;          // liczba plansz na strategię
    int liczba_agentow = 1;           // liczba agentów symulowanych podczas uczenia
    
    int moj_numer;
    Srodowisko s = new Srodowisko();  // środowisko obliczające nagrody oraz wyznaczające nowy stan
    Random loteria = new Random();    // generator liczb losowych

    int liczba_krokow_max = (s.liczba_kolumn + s.liczba_wierszy)*4;	// maksymalna liczba krokow w epizodzie

    long numer_epizodu = 0;

    UczenieZeWzm(int typ_strategii)
    {
        System.out.println("konstr UczenieZeWzmocnieniem - wersja a");
        Q = new float[s.liczba_wierszy][s.liczba_kolumn][5];
        for (int i = 0;i<s.liczba_wierszy;i++)
              for (int j = 0;j<s.liczba_kolumn;j++)
                  for (int a=0;a<5;a++)
                      Q[i][j][a] = 0;
        strategia = new Strategia(typ_strategii,s.liczba_wierszy,s.liczba_kolumn);
    }
    
    void epizod_uczenia_Qlearning()
    {
        StanInagroda stan = new StanInagroda();			// stan agenta (numer wiersza i kolumny)
        StanInagroda stan_nast = new StanInagroda();	// stan następny
        //System.out.printf("moj_num = %d, liczba_ag = %d\n",moj_numer,liczba_agentow);
        
        StanInagroda polozenia_agentow[] = new StanInagroda[100];	// położenia agentów
        for (int j=0;j<100;j++)
            polozenia_agentow[j] = new StanInagroda();
        
        // losujemy położenia początkowe wszystkich symulowanych agentów:
        for (int ag=0;ag<liczba_agentow;ag++)
        {
            polozenia_agentow[ag] = s.losowanie_stanu_pocz(polozenia_agentow, ag);
            // if (ag == moj_numer) polozenia_agentow[ag] = ...   // standardowo metoda s.losowanie_stanu_pocz losuje
            // ................................................   // stan początkowy w pierwszej kolumnie. Można tego nie zmieniać,
            // ................................................   // ale podczas uczenia czasami lepiej jest losować z całej planszy
            // ................................................
            // ................................................

            //System.out.printf("dla ag. %d wylosowano następujące liczby: [%d, %d]",ag,polozenia_agentow[ag][0],polozenia_agentow[ag][1]);
        }

        stan.w = polozenia_agentow[moj_numer].w;  // stan naszego agenta
        stan.k = polozenia_agentow[moj_numer].k;

        int nr_kroku = 0;
        float suma_nagrod = 0;
        
        // ustalenie akcji o najwyższej wartości w aktualnym przybliżeniu optymalnej strategii:
        double Qmax = -1e10;
        int nr_akcji_Qmax = -1;
        for (int j=0;j<5;j++) {
            if (Qmax < Q[stan.w][stan.k][j]) {
                Qmax = Q[stan.w][stan.k][j];
                nr_akcji_Qmax = j;
            }
        }

        double nagroda = -1;
        while ((stan.k != s.liczba_kolumn-1)&&(nr_kroku < liczba_krokow_max)&&(nagroda <= 0))
        {
            // losowanie eksploracji (jest niezbędna ze względu na konieczność sprawdzenia akcji, które nie mają jeszcze wiarygodnej oceny):
            boolean eksploracja = (loteria.nextFloat() < epsylon);  // ekspolarcja epsylon-zachłanna

            int nr_akcji_wyb = 0;
            //int nr_akcji_wyb = ...                                // wybór najlepszej znanej akcji
            //..................................................    // np. met. epsylon-zachłanną lub proporcjonalnie do użyteczności (wartości Q)
            //..................................................
            //..................................................
            //..................................................
            //..................................................


            // mieszanie numerów agentów by uwzględnić asynchroniczność i przypadkowość wykonywania akcji przez agentów
            int tab_kolejnosci_agentow[] = new int[liczba_agentow];
            for (int ag=0;ag<liczba_agentow;ag++) tab_kolejnosci_agentow[ag] = ag;
            for (int ag=0;ag<liczba_agentow;ag++) {                     // losowanie ze zwracaniem
                int los = ag+loteria.nextInt(liczba_agentow-ag);
                int ag_los = tab_kolejnosci_agentow[los];
                tab_kolejnosci_agentow[los] = tab_kolejnosci_agentow[ag];
                tab_kolejnosci_agentow[ag] = ag_los;
            }


            // wykonanie wybranej akcji:
            for (int ag=0;ag<liczba_agentow;ag++)
            {
                int nr_agenta = tab_kolejnosci_agentow[ag];

                StanInagroda st = new StanInagroda();
                st.w = polozenia_agentow[nr_agenta].w;
                st.k = polozenia_agentow[nr_agenta].k;

                int nr_akcji = strategie_wszystkie[nr_agenta].nry_akcji[0][st.w][st.k];
                if (nr_agenta == moj_numer)
                    nr_akcji = nr_akcji_wyb;

                StanInagroda stan_i_nagroda = s.ruch_agenta(st,nr_akcji,polozenia_agentow,liczba_agentow);

                polozenia_agentow[nr_agenta].w = stan_i_nagroda.w;
                polozenia_agentow[nr_agenta].k = stan_i_nagroda.k;

                if (nr_agenta == moj_numer)
                {
                    stan_nast.w = stan_i_nagroda.w;
                    stan_nast.k = stan_i_nagroda.k;
                    nagroda = stan_i_nagroda.nagroda;
                    suma_nagrod += nagroda;
                }
            }

            // ustalenie akcji o najwyższej wartości w następnym stanie:
            Qmax = -1e10;
            for (int i=0;i<5;i++)
                if (Qmax < Q[stan_nast.w][stan_nast.k][i])
                {
                    Qmax = Q[stan_nast.w][stan_nast.k][i];
                    nr_akcji_Qmax = i;
                }

            // modyfikacja wartości użyteczności poprzedniej akcji:
            // Q[stan.w][stan.k][nr_akcji_wyb] = ...
            //...................................................
            //...................................................
            //...................................................
            //...................................................
            
            // przejście do kolejnego stanu:
            stan.w = stan_nast.w;      // numer wiersza
            stan.k = stan_nast.k;      // numer kolumny
            
            nr_kroku ++;
        } // while po krokach epizodu
       numer_epizodu++;
    } // metoda epizod_uczenia_Qlearning()

    void tworz_strategie()
    {
        // tworzenie strategii na podstawie tablicy Q
        for (int i = 0;i<s.liczba_wierszy;i++)
              for (int j = 0;j<s.liczba_kolumn;j++)
              {
                  double Qmax = -1e10;
                  int nr_akcji_Qmax = -1;
                  for (int a=0;a<5;a++) {
                      if (Qmax < Q[i][j][a]) {
                          Qmax = Q[i][j][a];
                          nr_akcji_Qmax = a;
                      }
                  }
                  strategia.nry_akcji[0][i][j] = nr_akcji_Qmax;
              }
    }
    
    void zapis_Q_do_pliku(int numer_agenta)
    {
        FileOutputStream fos = null;


        File file = new File("Qagenta" +numer_agenta+ ".txt");

        try {
            fos = new FileOutputStream(file);

        } catch (FileNotFoundException fnfe) {
            System.out.println("Brak pliku!");
        }

        DataOutputStream dos = new DataOutputStream(fos);

        try {    //zapisywanie liczb z i-tej kolumny do jednego pliku
            for (int w = 0; w < s.liczba_wierszy; w++) {
                for (int k = 0; k < s.liczba_kolumn; k++) {
                    dos.writeBytes("nr wiersza " + w + ", nr kolumny " + k + ", śr. suma nagród Q = [");
                    for (int a = 0; a < 5; a++) {
                        dos.writeBytes(Q[w][k][a] + ", ");
                    }
                    dos.writeBytes("]\n");
                }
            }
            dos.writeBytes("\nstrategia:\n");
            for (int w = 0; w < s.liczba_wierszy; w++) {
                for (int k = 0; k < s.liczba_kolumn; k++) {
                    dos.writeBytes(" "+strategia.nry_akcji[0][w][k]);
                }
                dos.writeBytes("\n");
            }
        } catch (IOException ioe) {
            System.out.println("Blad zapisu!");

        }

    }
}
