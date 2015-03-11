/************************************************************
Interakcja:
Wysy³anie, odbiór komunikatów, interakcja z innymi
uczestnikami WZR, sterowanie wirtualnymi obiektami  
*************************************************************/

bool czy_opoznienia = 0;            // symulacja opóŸnieñ w sieci 
bool czy_zmn_czestosc = 1;          // symulacja ograniczonej czêstoœci (przepustowoœci) wysy³ania ramek  
bool czy_test_pred = 0;             // testowanie algorytmu predykcji bez udzia³u cz³owieka

#include <windows.h>
#include <time.h>
#include <stdio.h>   
#include "interakcja.h"
#include "obiekty.h"
#include "siec.h"

FILE *f = fopen("wzr_plik.txt","w");    // plik do zapisu informacji testowych


ObiektRuchomy *pMojObiekt;          // obiekt przypisany do tej aplikacji
Teren teren;
int iLiczbaCudzychOb = 0;
ObiektRuchomy *CudzeObiekty[1000];  // obiekty z innych aplikacji lub inne obiekty niz pCraft
int IndeksyOb[5000];                // tablica indeksow innych obiektow ulatwiajaca wyszukiwanie

float fDt;                          // sredni czas pomiedzy dwoma kolejnymi cyklami symulacji i wyswietlania
long czas_cyklu_WS,licznik_sym;     // zmienne pomocnicze potrzebne do obliczania fDt
float sr_czestosc;                  // srednia czestosc wysylania ramek w [ramkach/s] 
float sum_roznic_pol=0;             // sumaryczna odleg³oœæ pomiêdzy po³o¿eniem rzeczywistym (symulowanym) a ekstrapolowanym
float sum_roznic_kat=0;             // sumaryczna ró¿nica k¹towa -||- 
long czas_start = clock();          // czas uruchomienia aplikacji

multicast_net *multi_reciv;         // wsk do obiektu zajmujacego sie odbiorem komunikatow
multicast_net *multi_send;          //   -||-  wysylaniem komunikatow

HANDLE threadReciv;                 // uchwyt w¹tku odbioru komunikatów
extern HWND okno;       
int SHIFTwcisniety = 0;
bool czy_rysowac_ID = 1;            // czy rysowac nr ID przy ka¿dym obiekcie
bool sterowanie_myszkowe = 0;       // sterowanie za pomoc¹ klawisza myszki
int kursor_x = 0, kursor_y = 0;     // po³o¿enie kursora myszy

// Parametry widoku:
Wektor3 kierunek_kamery_1 = Wektor3(10,-3,-14),   // kierunek patrzenia
pol_kamery_1 = Wektor3(-35,6,10),         // po³o¿enie kamery
pion_kamery_1 = Wektor3(0,1,0),           // kierunek pionu kamery        
kierunek_kamery_2 = Wektor3(0,-1,0.02),   // to samo dla widoku z góry
pol_kamery_2 = Wektor3(0,100,0),
pion_kamery_2 = Wektor3(0,0,-1),
kierunek_kamery = kierunek_kamery_1, pol_kamery = pol_kamery_1, pion_kamery = pion_kamery_1; 
bool sledzenie = 0;                             // tryb œledzenia obiektu przez kamerê
bool widok_z_gory = 0;                          // tryb widoku z góry
float oddalenie = 4.0;                          // oddalenie widoku z kamery
float kat_kam_z = 0;                            // obrót kamery góra-dó³
float oddalenie_1 = oddalenie, kat_kam_z_1 = kat_kam_z, oddalenie_2 = oddalenie, kat_kam_z_2 = kat_kam_z,
oddalenie_3 = oddalenie, kat_kam_z_3 = kat_kam_z;

// Scenariusz testu ekstrapolacji: 
float scenariusz_testu[][3] = {{5, 30, 0}, {5, 30, PI*20/180}, {5, 30, -PI*20/180}, {5, -10, 0}}; // {czas, predkoœæ, k¹t skrêtu kó³} -> przez jaki czas obiekt ma sie poruszaæ z podan¹ prêdkoœci¹ i k¹tem skrêtu kó³
long nr_akcji = 0;


struct Ramka                                    // g³ówna struktura s³u¿¹ca do przesy³ania informacji
{
  int typ;
  StanObiektu stan;                            
  long moment_wyslania;                        // tzw. znacznik czasu potrzebny np. do obliczenia opóŸnienia
};


//******************************************
// Funkcja obs³ugi w¹tku odbioru komunikatów 
// UWAGA!  Odbierane s¹ te¿ komunikaty z w³asnej aplikacji by porównaæ obraz ekstrapolowany do rzeczywistego.
DWORD WINAPI WatekOdbioru(void *ptr)
{
  multicast_net *pmt_net=(multicast_net*)ptr;  // wskaŸnik do obiektu klasy multicast_net
  int rozmiar;                                 // liczba bajtów ramki otrzymanej z sieci
  Ramka ramka;
  StanObiektu stan;

  while(1)
  {
    rozmiar = pmt_net->reciv((char*)&ramka,sizeof(Ramka));   // oczekiwanie na nadejœcie ramki 
    stan = ramka.stan;

    //fprintf(f,"odebrano stan iID = %d, ID dla mojego obiektu = %d\n",stan.iID,pMojObiekt->iID);

    //if (stan.iID != pMojObiekt->iID)          // jeœli to nie mój w³asny obiekt
    {
      if (IndeksyOb[stan.iID] == -1)        // nie ma jeszcze takiego obiektu w tablicy -> trzeba go
                                            // stworzyæ
      {
        CudzeObiekty[iLiczbaCudzychOb] = new ObiektRuchomy();   
        IndeksyOb[stan.iID] = iLiczbaCudzychOb;     // wpis do tablicy indeksowanej numerami ID
        // u³atwia wyszukiwanie, alternatyw¹ mo¿e byæ tabl. rozproszona           
        //fprintf(f,"zarejestrowano %d obcy obiekt o ID = %d\n",iLiczbaCudzychOb-1,CudzeObiekty[iLiczbaCudzychOb]->iID);                                                                                      
        iLiczbaCudzychOb++;     
      }                                                                    
      CudzeObiekty[IndeksyOb[stan.iID]]->ZmienStan(stan);   // aktualizacja stanu obiektu obcego 			
    }
  }  // while(1)
  return 1;
}

// *****************************************************************
// ****    Wszystko co trzeba zrobiæ podczas uruchamiania aplikacji
// ****    poza grafik¹   
void PoczatekInterakcji()
{
  DWORD dwThreadId;

  pMojObiekt = new ObiektRuchomy();    // tworzenie wlasnego obiektu

  for (long i=0;i<4000;i++)            // inicjacja indeksow obcych obiektow
    IndeksyOb[i] = -1;

  czas_cyklu_WS = clock();             // pomiar aktualnego czasu

  // obiekty sieciowe typu multicast (z podaniem adresu WZR oraz numeru portu)
  multi_reciv = new multicast_net("224.12.12.99",10001);      // obiekt do odbioru ramek sieciowych
  multi_send = new multicast_net("224.12.12.99",10001);       // obiekt do wysy³ania ramek

  if (czy_opoznienia)
  {
    float srednie_opoznienie = 2*(float)rand()/RAND_MAX, wariancja_opoznienia = 0.6; // parametry opóŸnieñ
    multi_send->PrepareDelay(srednie_opoznienie,wariancja_opoznienia);               // ustawienie opóŸnieñ
  }

  // uruchomienie watku obslugujacego odbior komunikatow
  threadReciv = CreateThread(
    NULL,                        // no security attributes
    0,                           // use default stack size
    WatekOdbioru,                // thread function
    (void *)multi_reciv,               // argument to thread function
    0,                           // use default creation flags
    &dwThreadId);                // returns the thread identifier

}


// *****************************************************************
// ****    Wszystko co trzeba zrobiæ w ka¿dym cyklu dzia³ania 
// ****    aplikacji poza grafik¹ 
void Cykl_WS()
{
  licznik_sym++;  

  // obliczenie sumarycznej odleg³oœci pomiêdzy pojazdem ekstrapolowanym a rzeczywistym:
  long indeks_cienia = -1;
  for (long i=0;i<iLiczbaCudzychOb;i++) if (CudzeObiekty[i]->iID == pMojObiekt->iID) indeks_cienia = i;
  sum_roznic_pol += (indeks_cienia > -1 ? (CudzeObiekty[indeks_cienia]->wPol - pMojObiekt->wPol).dlugosc() : 0);

  // obliczenie œredniej ró¿nicy k¹towej:
  float kat = (indeks_cienia > -1 ? 
    fabs(kat_pom_wekt2D(CudzeObiekty[indeks_cienia]->qOrient.obroc_wektor(Wektor3(1,0,0)),pMojObiekt->qOrient.obroc_wektor(Wektor3(1,0,0)) )) : 0);
  kat = (fabs(kat-2*3.14159) ? kat > 3.14159 : kat);
  sum_roznic_kat += kat;

  if (licznik_sym % 50 == 0)          // jeœli licznik cykli przekroczy³ pewn¹ wartoœæ, to
  {                              // nale¿y na nowo obliczyæ œredni czas cyklu fDt
    char text[200];
    long czas_pop = czas_cyklu_WS;
    czas_cyklu_WS = clock();
    float fFps = (50*CLOCKS_PER_SEC)/(float)(czas_cyklu_WS-czas_pop);
    if (fFps!=0) fDt=1.0/fFps; else fDt=1;

    sprintf(text," %0.0f fps  %0.2fms  œr.czêstoœæ = %0.2f[r/s]  œr.odl = %0.3f[m]  œr.ro¿n.k¹t. = %0.3f[st]",
      fFps,1000.0/fFps,sr_czestosc,sum_roznic_pol/licznik_sym,sum_roznic_kat/licznik_sym*180.0/3.14159);
    SetWindowText(okno,text); // wyœwietlenie aktualnej iloœci klatek/s w pasku okna			
  }   

  if (czy_test_pred)  // obs³uga testu predykcji dla wybranego scenariusza
  {
    float czas = (float)(clock()-czas_start)/CLOCKS_PER_SEC;
    long liczba_akcji = sizeof(scenariusz_testu)/(3*sizeof(float));
    float suma_czasow = 0;

    long nr_akcji = -1;
    for (long i=0;i<liczba_akcji;i++)
    {
      suma_czasow += scenariusz_testu[i][0];
      if (czas < suma_czasow) {nr_akcji = i; break;}
    }

    fprintf(f,"liczba akcji = %d, czas = %f, nr akcji = %d\n",liczba_akcji,czas,nr_akcji);

    if (nr_akcji > -1) // jesli wyznaczono nr akcji, wybieram sile i kat ze scenariusza
    {
      pMojObiekt->F = scenariusz_testu[nr_akcji][1]; 
      pMojObiekt->alfa = scenariusz_testu[nr_akcji][2];
    }
    else // czas dobiegl konca -> koniec testu 
    {
      czy_test_pred = false;
      char text[200];
      sprintf(text,"Po czasie %3.2f[s]  œr.czêstoœæ = %0.2f[r/s]  œr.odl = %0.3f[m]  œr.ró¿n.k¹t. = %0.3f[st]",
        czas,sr_czestosc,sum_roznic_pol/licznik_sym,sum_roznic_kat/licznik_sym*180.0/3.14159);
      MessageBox(okno,text,"Test predykcji",MB_OK);		
    }
  } // if test predykcji

  pMojObiekt->Symulacja(fDt);                    // symulacja w³asnego obiektu

  Ramka ramka;                                    
  ramka.stan = pMojObiekt->Stan();               // stan w³asnego obiektu 
  ramka.moment_wyslania = clock();


  // wys³anie komunikatu o stanie obiektu przypisanego do aplikacji (pMojObiekt):    
  if ((licznik_sym % 100 == 0) || !czy_zmn_czestosc)
    int iRozmiar = multi_send->send_delayed((char*)&ramka,sizeof(Ramka));

  // wysy³anie komunikatu z normaln¹ czêstoœci¹ i bez opóŸnieñ:
  //multi_send->send((char*)&ramka,sizeof(Ramka));

  //       ----------------------------------
  //    -------------------------------------
  // ----------------------------------------
  // ------------  Miejsce na predykcjê stanu:
  for (int k=0;k<iLiczbaCudzychOb;k++)
  {
	  //CudzeObiekty[k]->wPol += CudzeObiekty[k]->wV * fDt * (twA * fDt * fDt / 2);
	  CudzeObiekty[k]->wPol += CudzeObiekty[k]->wA * fDt;

	  float kat = CudzeObiekty[k]->wV_kat * fDt + (CudzeObiekty[k]->wA_kat * (fDt * fDt / 2)).dlugosc();
	  CudzeObiekty[k]->qOrient = AsixToQuat(CudzeObiekty[k]->wV_kat, kat);
	  CudzeObiekty[k]->wV_kat = CudzeObiekty[k]->wA_kat * fDt;




  } 

}

// *****************************************************************
// ****    Wszystko co trzeba zrobiæ podczas zamykania aplikacji
// ****    poza grafik¹ 
void ZakonczenieInterakcji()
{
  fprintf(f,"Koniec interakcji\n");
  fclose(f);
}


// ************************************************************************
// ****    Obs³uga klawiszy s³u¿¹cych do sterowania obiektami lub
// ****    widokami 
void KlawiszologiaSterowania(UINT kod_meldunku, WPARAM wParam, LPARAM lParam)
{

  switch (kod_meldunku) 
  {

  case WM_LBUTTONDOWN: //reakcja na lewy przycisk myszki
    {
      int x = LOWORD(lParam);
      int y = HIWORD(lParam);
      if (sterowanie_myszkowe)
        pMojObiekt->F = 30.0;        // si³a pchaj¹ca do przodu
      break;
    }
  case WM_RBUTTONDOWN: //reakcja na prawy przycisk myszki
    {
      int x = LOWORD(lParam);
      int y = HIWORD(lParam);
      if (sterowanie_myszkowe)
        pMojObiekt->F = -5.0;        // si³a pchaj¹ca do tylu
      break;
    }
  case WM_MBUTTONDOWN: //reakcja na œrodkowy przycisk myszki : uaktywnienie/dezaktywacja sterwania myszkowego
    {
      sterowanie_myszkowe = 1 - sterowanie_myszkowe;
      kursor_x = LOWORD(lParam);
      kursor_y = HIWORD(lParam);
      break;
    }
  case WM_LBUTTONUP: //reakcja na puszczenie lewego przycisku myszki
    {   
      if (sterowanie_myszkowe)
        pMojObiekt->F = 0.0;        // si³a pchaj¹ca do przodu
      break;
    }
  case WM_RBUTTONUP: //reakcja na puszczenie lewy przycisk myszki
    {
      if (sterowanie_myszkowe)
        pMojObiekt->F = 0.0;        // si³a pchaj¹ca do przodu
      break;
    }
  case WM_MOUSEMOVE:
    {
      int x = LOWORD(lParam);
      int y = HIWORD(lParam);
      if (sterowanie_myszkowe)
      {
        float kat_skretu = (float)(kursor_x - x)/20;
        if (kat_skretu > 45) kat_skretu = 45;
        if (kat_skretu < -45) kat_skretu = -45;	
        pMojObiekt->alfa = PI*kat_skretu/180;
      }
      break;
    }
  case WM_KEYDOWN:
    {

      switch (LOWORD(wParam))
      {
      case VK_SHIFT:
        {
          SHIFTwcisniety = 1; 
          break; 
        }        
      case VK_SPACE:
        {
          pMojObiekt->ham = 10.0;       // stopieñ hamowania (reszta zale¿y od si³y docisku i wsp. tarcia)
          break;                       // 1.0 to maksymalny stopieñ (np. zablokowanie kó³)
        }
      case VK_UP:
        {

          pMojObiekt->F = 30.0;        // si³a pchaj¹ca do przodu
          break;
        }
      case VK_DOWN:
        {
          pMojObiekt->F = -5.0;
          break;
        }
      case VK_LEFT:
        {
          if (SHIFTwcisniety) pMojObiekt->alfa = PI*25/180;
          else pMojObiekt->alfa = PI*10/180;

          break;
        }
      case VK_RIGHT:
        {
          if (SHIFTwcisniety) pMojObiekt->alfa = -PI*25/180;
          else pMojObiekt->alfa = -PI*10/180;
          break;
        }
      case 'I':   // wypisywanie nr ID
        {
          czy_rysowac_ID = 1 - czy_rysowac_ID;
          break;
        }
      case 'W':   // oddalenie widoku
        {
          //pol_kamery = pol_kamery - kierunek_kamery*0.3;
          if (oddalenie > 0.5) oddalenie /= 1.2;
          else oddalenie = 0;  
          break; 
        }     
      case 'S':   // przybli¿enie widoku
        {
          //pol_kamery = pol_kamery + kierunek_kamery*0.3; 
          if (oddalenie > 0) oddalenie *= 1.2;
          else oddalenie = 0.5;   
          break; 
        }    
      case 'Q':   // widok z góry
        {
          if (sledzenie) break;
          widok_z_gory = 1-widok_z_gory;
          if (widok_z_gory)
          {
            pol_kamery_1 = pol_kamery; kierunek_kamery_1 = kierunek_kamery; pion_kamery_1 = pion_kamery;
            oddalenie_1 = oddalenie; kat_kam_z_1 = kat_kam_z; 
            pol_kamery = pol_kamery_2; kierunek_kamery = kierunek_kamery_2; pion_kamery = pion_kamery_2;
            oddalenie = oddalenie_2; kat_kam_z = kat_kam_z_2; 
          }
          else
          {
            pol_kamery_2 = pol_kamery; kierunek_kamery_2 = kierunek_kamery; pion_kamery_2 = pion_kamery;
            oddalenie_2 = oddalenie; kat_kam_z_2 = kat_kam_z;
            pol_kamery = pol_kamery_1; kierunek_kamery = kierunek_kamery_1; pion_kamery = pion_kamery_1;
            oddalenie = oddalenie_1; kat_kam_z = kat_kam_z_1; 
          }
          break;
        }
      case 'E':   // obrót kamery ku górze (wzglêdem lokalnej osi z)
        {
          kat_kam_z += PI*5/180; 
          break; 
        }    
      case 'D':   // obrót kamery ku do³owi (wzglêdem lokalnej osi z)
        {
          kat_kam_z -= PI*5/180;  
          break;
        }
      case 'A':   // w³¹czanie, wy³¹czanie trybu œledzenia obiektu
        {
          sledzenie = 1 - sledzenie;
          if (sledzenie)
          {
            oddalenie = oddalenie_3; kat_kam_z = kat_kam_z_3; 
          }
          else
          {
            oddalenie_3 = oddalenie; kat_kam_z_3 = kat_kam_z; 
            widok_z_gory = 0;
            pol_kamery = pol_kamery_1; kierunek_kamery = kierunek_kamery_1; pion_kamery = pion_kamery_1;
            oddalenie = oddalenie_1; kat_kam_z = kat_kam_z_1; 
          }
          break;
        }
      case VK_F1:  // wywolanie systemu pomocy
        {
          char lan[1024],lan_bie[1024];
          //GetSystemDirectory(lan_sys,1024);
          GetCurrentDirectory(1024,lan_bie);
          strcpy(lan,"C:\\Program Files\\Internet Explorer\\iexplore ");
          strcat(lan,lan_bie);
          strcat(lan,"\\pomoc.htm");
          int wyni = WinExec(lan,SW_NORMAL);
          if (wyni < 32)  // proba uruchominia pomocy nie powiodla sie
          {
            strcpy(lan,"C:\\Program Files\\Mozilla Firefox\\firefox ");
            strcat(lan,lan_bie);
            strcat(lan,"\\pomoc.htm");
            wyni = WinExec(lan,SW_NORMAL);
            if (wyni < 32)
            {
              char lan_win[1024];
              GetWindowsDirectory(lan_win,1024);
              strcat(lan_win,"\\notepad pomoc.txt ");
              wyni = WinExec(lan_win,SW_NORMAL);
            }
          }
          break;
        }
      case VK_ESCAPE:
        {
          SendMessage(okno, WM_DESTROY,0,0);
          break;
        }
      } // switch po klawiszach

      break;
    }
  case WM_KEYUP:
    {
      switch (LOWORD(wParam))
      {
      case VK_SHIFT:
        {
          SHIFTwcisniety = 0; 
          break; 
        }        
      case VK_SPACE:
        {
          pMojObiekt->ham = 0.0;
          break;
        }
      case VK_UP:
        {
          pMojObiekt->F = 0.0;

          break;
        }
      case VK_DOWN:
        {
          pMojObiekt->F = 0.0;
          break;
        }
      case VK_LEFT:
        {
          pMojObiekt->Fb = 0.00;
          pMojObiekt->alfa = 0;	
          break;
        }
      case VK_RIGHT:
        {
          pMojObiekt->Fb = 0.00;
          pMojObiekt->alfa = 0;	
          break;
        }

      }

      break;
    }

  } // switch po komunikatach
}
