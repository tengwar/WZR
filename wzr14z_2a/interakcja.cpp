/************************************************************
Interakcja:
Wysy�anie, odbi�r komunikat�w, interakcja z innymi
uczestnikami WZR, sterowanie wirtualnymi obiektami  
*************************************************************/

bool czy_opoznienia = 0;            // symulacja op�nie� w sieci 
bool czy_zmn_czestosc = 1;          // symulacja ograniczonej cz�sto�ci (przepustowo�ci) wysy�ania ramek  
bool czy_test_pred = 0;             // testowanie algorytmu predykcji bez udzia�u cz�owieka

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
float sum_roznic_pol=0;             // sumaryczna odleg�o�� pomi�dzy po�o�eniem rzeczywistym (symulowanym) a ekstrapolowanym
float sum_roznic_kat=0;             // sumaryczna r�nica k�towa -||- 
long czas_start = clock();          // czas uruchomienia aplikacji

multicast_net *multi_reciv;         // wsk do obiektu zajmujacego sie odbiorem komunikatow
multicast_net *multi_send;          //   -||-  wysylaniem komunikatow

HANDLE threadReciv;                 // uchwyt w�tku odbioru komunikat�w
extern HWND okno;       
int SHIFTwcisniety = 0;
bool czy_rysowac_ID = 1;            // czy rysowac nr ID przy ka�dym obiekcie
bool sterowanie_myszkowe = 0;       // sterowanie za pomoc� klawisza myszki
int kursor_x = 0, kursor_y = 0;     // po�o�enie kursora myszy

// Parametry widoku:
Wektor3 kierunek_kamery_1 = Wektor3(10,-3,-14),   // kierunek patrzenia
pol_kamery_1 = Wektor3(-35,6,10),         // po�o�enie kamery
pion_kamery_1 = Wektor3(0,1,0),           // kierunek pionu kamery        
kierunek_kamery_2 = Wektor3(0,-1,0.02),   // to samo dla widoku z g�ry
pol_kamery_2 = Wektor3(0,100,0),
pion_kamery_2 = Wektor3(0,0,-1),
kierunek_kamery = kierunek_kamery_1, pol_kamery = pol_kamery_1, pion_kamery = pion_kamery_1; 
bool sledzenie = 0;                             // tryb �ledzenia obiektu przez kamer�
bool widok_z_gory = 0;                          // tryb widoku z g�ry
float oddalenie = 4.0;                          // oddalenie widoku z kamery
float kat_kam_z = 0;                            // obr�t kamery g�ra-d�
float oddalenie_1 = oddalenie, kat_kam_z_1 = kat_kam_z, oddalenie_2 = oddalenie, kat_kam_z_2 = kat_kam_z,
oddalenie_3 = oddalenie, kat_kam_z_3 = kat_kam_z;

// Scenariusz testu ekstrapolacji: 
float scenariusz_testu[][3] = {{5, 30, 0}, {5, 30, PI*20/180}, {5, 30, -PI*20/180}, {5, -10, 0}}; // {czas, predko��, k�t skr�tu k�} -> przez jaki czas obiekt ma sie porusza� z podan� pr�dko�ci� i k�tem skr�tu k�
long nr_akcji = 0;


struct Ramka                                    // g��wna struktura s�u��ca do przesy�ania informacji
{
  int typ;
  StanObiektu stan;                            
  long moment_wyslania;                        // tzw. znacznik czasu potrzebny np. do obliczenia op�nienia
};


//******************************************
// Funkcja obs�ugi w�tku odbioru komunikat�w 
// UWAGA!  Odbierane s� te� komunikaty z w�asnej aplikacji by por�wna� obraz ekstrapolowany do rzeczywistego.
DWORD WINAPI WatekOdbioru(void *ptr)
{
  multicast_net *pmt_net=(multicast_net*)ptr;  // wska�nik do obiektu klasy multicast_net
  int rozmiar;                                 // liczba bajt�w ramki otrzymanej z sieci
  Ramka ramka;
  StanObiektu stan;

  while(1)
  {
    rozmiar = pmt_net->reciv((char*)&ramka,sizeof(Ramka));   // oczekiwanie na nadej�cie ramki 
    stan = ramka.stan;

    //fprintf(f,"odebrano stan iID = %d, ID dla mojego obiektu = %d\n",stan.iID,pMojObiekt->iID);

    //if (stan.iID != pMojObiekt->iID)          // je�li to nie m�j w�asny obiekt
    {
      if (IndeksyOb[stan.iID] == -1)        // nie ma jeszcze takiego obiektu w tablicy -> trzeba go
                                            // stworzy�
      {
        CudzeObiekty[iLiczbaCudzychOb] = new ObiektRuchomy();   
        IndeksyOb[stan.iID] = iLiczbaCudzychOb;     // wpis do tablicy indeksowanej numerami ID
        // u�atwia wyszukiwanie, alternatyw� mo�e by� tabl. rozproszona           
        //fprintf(f,"zarejestrowano %d obcy obiekt o ID = %d\n",iLiczbaCudzychOb-1,CudzeObiekty[iLiczbaCudzychOb]->iID);                                                                                      
        iLiczbaCudzychOb++;     
      }                                                                    
      CudzeObiekty[IndeksyOb[stan.iID]]->ZmienStan(stan);   // aktualizacja stanu obiektu obcego 			
    }
  }  // while(1)
  return 1;
}

// *****************************************************************
// ****    Wszystko co trzeba zrobi� podczas uruchamiania aplikacji
// ****    poza grafik�   
void PoczatekInterakcji()
{
  DWORD dwThreadId;

  pMojObiekt = new ObiektRuchomy();    // tworzenie wlasnego obiektu

  for (long i=0;i<4000;i++)            // inicjacja indeksow obcych obiektow
    IndeksyOb[i] = -1;

  czas_cyklu_WS = clock();             // pomiar aktualnego czasu

  // obiekty sieciowe typu multicast (z podaniem adresu WZR oraz numeru portu)
  multi_reciv = new multicast_net("224.12.12.99",10001);      // obiekt do odbioru ramek sieciowych
  multi_send = new multicast_net("224.12.12.99",10001);       // obiekt do wysy�ania ramek

  if (czy_opoznienia)
  {
    float srednie_opoznienie = 2*(float)rand()/RAND_MAX, wariancja_opoznienia = 0.6; // parametry op�nie�
    multi_send->PrepareDelay(srednie_opoznienie,wariancja_opoznienia);               // ustawienie op�nie�
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
// ****    Wszystko co trzeba zrobi� w ka�dym cyklu dzia�ania 
// ****    aplikacji poza grafik� 
void Cykl_WS()
{
  licznik_sym++;  

  // obliczenie sumarycznej odleg�o�ci pomi�dzy pojazdem ekstrapolowanym a rzeczywistym:
  long indeks_cienia = -1;
  for (long i=0;i<iLiczbaCudzychOb;i++) if (CudzeObiekty[i]->iID == pMojObiekt->iID) indeks_cienia = i;
  sum_roznic_pol += (indeks_cienia > -1 ? (CudzeObiekty[indeks_cienia]->wPol - pMojObiekt->wPol).dlugosc() : 0);

  // obliczenie �redniej r�nicy k�towej:
  float kat = (indeks_cienia > -1 ? 
    fabs(kat_pom_wekt2D(CudzeObiekty[indeks_cienia]->qOrient.obroc_wektor(Wektor3(1,0,0)),pMojObiekt->qOrient.obroc_wektor(Wektor3(1,0,0)) )) : 0);
  kat = (fabs(kat-2*3.14159) ? kat > 3.14159 : kat);
  sum_roznic_kat += kat;

  if (licznik_sym % 50 == 0)          // je�li licznik cykli przekroczy� pewn� warto��, to
  {                              // nale�y na nowo obliczy� �redni czas cyklu fDt
    char text[200];
    long czas_pop = czas_cyklu_WS;
    czas_cyklu_WS = clock();
    float fFps = (50*CLOCKS_PER_SEC)/(float)(czas_cyklu_WS-czas_pop);
    if (fFps!=0) fDt=1.0/fFps; else fDt=1;

    sprintf(text," %0.0f fps  %0.2fms  �r.cz�sto�� = %0.2f[r/s]  �r.odl = %0.3f[m]  �r.ro�n.k�t. = %0.3f[st]",
      fFps,1000.0/fFps,sr_czestosc,sum_roznic_pol/licznik_sym,sum_roznic_kat/licznik_sym*180.0/3.14159);
    SetWindowText(okno,text); // wy�wietlenie aktualnej ilo�ci klatek/s w pasku okna			
  }   

  if (czy_test_pred)  // obs�uga testu predykcji dla wybranego scenariusza
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
      sprintf(text,"Po czasie %3.2f[s]  �r.cz�sto�� = %0.2f[r/s]  �r.odl = %0.3f[m]  �r.r�n.k�t. = %0.3f[st]",
        czas,sr_czestosc,sum_roznic_pol/licznik_sym,sum_roznic_kat/licznik_sym*180.0/3.14159);
      MessageBox(okno,text,"Test predykcji",MB_OK);		
    }
  } // if test predykcji

  pMojObiekt->Symulacja(fDt);                    // symulacja w�asnego obiektu

  Ramka ramka;                                    
  ramka.stan = pMojObiekt->Stan();               // stan w�asnego obiektu 
  ramka.moment_wyslania = clock();


  // wys�anie komunikatu o stanie obiektu przypisanego do aplikacji (pMojObiekt):    
  if ((licznik_sym % 100 == 0) || !czy_zmn_czestosc)
    int iRozmiar = multi_send->send_delayed((char*)&ramka,sizeof(Ramka));

  // wysy�anie komunikatu z normaln� cz�sto�ci� i bez op�nie�:
  //multi_send->send((char*)&ramka,sizeof(Ramka));

  //       ----------------------------------
  //    -------------------------------------
  // ----------------------------------------
  // ------------  Miejsce na predykcj� stanu:
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
// ****    Wszystko co trzeba zrobi� podczas zamykania aplikacji
// ****    poza grafik� 
void ZakonczenieInterakcji()
{
  fprintf(f,"Koniec interakcji\n");
  fclose(f);
}


// ************************************************************************
// ****    Obs�uga klawiszy s�u��cych do sterowania obiektami lub
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
        pMojObiekt->F = 30.0;        // si�a pchaj�ca do przodu
      break;
    }
  case WM_RBUTTONDOWN: //reakcja na prawy przycisk myszki
    {
      int x = LOWORD(lParam);
      int y = HIWORD(lParam);
      if (sterowanie_myszkowe)
        pMojObiekt->F = -5.0;        // si�a pchaj�ca do tylu
      break;
    }
  case WM_MBUTTONDOWN: //reakcja na �rodkowy przycisk myszki : uaktywnienie/dezaktywacja sterwania myszkowego
    {
      sterowanie_myszkowe = 1 - sterowanie_myszkowe;
      kursor_x = LOWORD(lParam);
      kursor_y = HIWORD(lParam);
      break;
    }
  case WM_LBUTTONUP: //reakcja na puszczenie lewego przycisku myszki
    {   
      if (sterowanie_myszkowe)
        pMojObiekt->F = 0.0;        // si�a pchaj�ca do przodu
      break;
    }
  case WM_RBUTTONUP: //reakcja na puszczenie lewy przycisk myszki
    {
      if (sterowanie_myszkowe)
        pMojObiekt->F = 0.0;        // si�a pchaj�ca do przodu
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
          pMojObiekt->ham = 10.0;       // stopie� hamowania (reszta zale�y od si�y docisku i wsp. tarcia)
          break;                       // 1.0 to maksymalny stopie� (np. zablokowanie k�)
        }
      case VK_UP:
        {

          pMojObiekt->F = 30.0;        // si�a pchaj�ca do przodu
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
      case 'S':   // przybli�enie widoku
        {
          //pol_kamery = pol_kamery + kierunek_kamery*0.3; 
          if (oddalenie > 0) oddalenie *= 1.2;
          else oddalenie = 0.5;   
          break; 
        }    
      case 'Q':   // widok z g�ry
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
      case 'E':   // obr�t kamery ku g�rze (wzgl�dem lokalnej osi z)
        {
          kat_kam_z += PI*5/180; 
          break; 
        }    
      case 'D':   // obr�t kamery ku do�owi (wzgl�dem lokalnej osi z)
        {
          kat_kam_z -= PI*5/180;  
          break;
        }
      case 'A':   // w��czanie, wy��czanie trybu �ledzenia obiektu
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
