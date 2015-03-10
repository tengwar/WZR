/************************************************************
	Interakcja:
	Wysy�anie, odbi�r komunikat�w, interakcja z innymi
	uczestnikami WZR, sterowanie wirtualnymi obiektami
	*************************************************************/

#include <windows.h>
#include <time.h>
#include <stdio.h>   
#include "interakcja.h"
#include "obiekty.h"

#include "siec.h"

FILE *f = fopen("plik.txt", "w");    // plik do zapisu informacji testowych


ObiektRuchomy *pMojObiekt;          // obiekt przypisany do tej aplikacji
Teren teren;
int iLiczbaInnychOb = 0;
ObiektRuchomy *InneObiekty[2000];  // obiekty z innych aplikacji lub inne obiekty niz pMojObiekt
int IndeksyObiektow[2000];                // tablica indeksow innych obiektow ulatwiajaca wyszukiwanie

float fDt;                          // sredni czas pomiedzy dwoma kolejnymi cyklami symulacji i wyswietlania
long czas_cyklu_WS, licznik_sym;     // zmienne pomocnicze potrzebne do obliczania fDt
float sr_czestosc;                  // srednia czestosc wysylania ramek w [ramkach/s]

multicast_net *multi_reciv;         // wsk do obiektu zajmujacego sie odbiorem komunikatow
multicast_net *multi_send;          //   -||-  wysylaniem komunikatow
unicast_net* uni_recv;// = new unicast_net(1001);
unicast_net *uni_send;

HANDLE threadReciv;                 // uchwyt w�tku odbioru komunikat�w
extern HWND okno;                   // uchwyt okna
int SHIFTwcisniety = 0;

// Parametry widoku:
Wektor3 kierunek_kamery = Wektor3(5, -3, -14);   // kierunek patrzenia
Wektor3 pol_kamery = Wektor3(-50, 7, 10);          // po�o�enie kamery
Wektor3 pion_kamery = Wektor3(0, 1, 0);           // kierunek pionu kamery             
bool sledzenie = 0;                             // tryb �ledzenia obiektu przez kamer�
bool widok_od_gory = 0;
float oddalenie = 2.0;                          // oddalenie widoku z kamery
float kat_kam_z = 0;                            // obr�t kamery g�ra-d�
float zoom = 1;                                 // zoom - zmniejszanie/zwi�kszanie ogniskowej obiektywu

int opoznienia = 0;


struct Ramka                                    // podstawowa struktura komunikacyjna
{
	StanObiektu stan;                            // po�o�enie, pr�dko��, itd.
	int typ;                                     // typ ramki - np. informacja o stanie, wiadomo�� tekstowa, itd. 
	long czas_wyslania;
};

char * ip = "192.168.1.2";
unsigned long adr;

//******************************************
// Funkcja obs�ugi w�tku odbioru komunikat�w 
DWORD WINAPI WatekOdbioruKom(void *ptr)
{
	unicast_net *pmt_net = (unicast_net*)ptr;  // wska�nik do obiektu klasy multicast_net
	int rozmiar;                                 // liczba bajt�w ramki otrzymanej z sieci
	Ramka ramka;
	StanObiektu stan;

	while (1)
	{

		rozmiar = pmt_net->reciv((char*)&ramka, &adr, sizeof(Ramka));   // oczekiwanie na nadej�cie ramki 
		stan = ramka.stan;

		//fprintf(f,"odebrano stan iID = %d, ID dla mojego obiektu = %d\n",stan.iID,pMojObiekt->iID);

		if (stan.iID != pMojObiekt->iID)          // je�li to nie m�j w�asny obiekt
		{
			if (IndeksyObiektow[stan.iID] == -1)        // nie ma jeszcze takiego obiektu w tablicy -> trzeba go
				// stworzy�
			{
				InneObiekty[iLiczbaInnychOb] = new ObiektRuchomy();
				IndeksyObiektow[stan.iID] = iLiczbaInnychOb;     // wpis do tablicy indeksowanej numerami ID
				// u�atwia wyszukiwanie, alternatyw� mo�e by� tabl. rozproszona           
				fprintf(f, "zarejestrowano %d obcy obiekt o ID = %d\n", iLiczbaInnychOb - 1, InneObiekty[iLiczbaInnychOb]->iID);

				iLiczbaInnychOb++;

			}
			InneObiekty[IndeksyObiektow[stan.iID]]->ZmienStan(stan);   // aktualizacja stanu obiektu obcego 			
		}
	}  // koniec while(1)
	return 1;
}

// *****************************************************************
// ****      Pocz�tkowe ustawienia:
// ****    Wszystko co trzeba zrobi� podczas uruchamiania aplikacji
// ****    poza grafik�   
void PoczatekInterakcji()
{
	DWORD dwThreadId;

	pMojObiekt = new ObiektRuchomy();    // tworzenie wlasnego obiektu

	for (long i = 0; i < 2000; i++)            // inicjacja indeksow obcych obiektow
		IndeksyObiektow[i] = -1;

	czas_cyklu_WS = clock();             // pomiar aktualnego czasu
	licznik_sym = 1;

	// obiekty sieciowe typu multicast (z podaniem adresu IP wirtualnej grupy oraz numeru portu)
	multi_reciv = new multicast_net("224.12.12.20", 10001);      // obiekt do odbioru ramek sieciowych
	multi_send = new multicast_net("224.12.12.20", 10001);       // obiekt do wysy�ania ramek
	uni_recv = new unicast_net(1001);
	uni_send = new unicast_net(1002);

	// uruchomienie watku obslugujacego odbior komunikatow 
	threadReciv = CreateThread(
		NULL,                        // no security attributes
		0,                           // use default stack size
		WatekOdbioruKom,                // thread function
		(void *)uni_recv,               // argument to thread function
		0,                           // use default creation flags
		&dwThreadId);                // returns the thread identifier         
}


// *****************************************************************
// ****    Wszystko co trzeba zrobi� w ka�dym cyklu dzia�ania 
// ****    aplikacji poza grafik� 
void Cykl_WS()
{
	licznik_sym++;

	// Wyznaczanie �redniego czasu (fDt) pomi�dzy dwoma kolejnymi symulacjami
	if (licznik_sym % 50 == 0)          // je�li licznik cykli przekroczy� pewn� warto��, to
	{                                   // nale�y na nowo obliczy� �redni czas cyklu fDt
		char text[200];
		long czas_pop = czas_cyklu_WS;
		czas_cyklu_WS = clock();
		float fFps = (50 * CLOCKS_PER_SEC) / (float)(czas_cyklu_WS - czas_pop);
		if (fFps != 0) fDt = 1.0 / fFps; else fDt = 1.0;

		if (licznik_sym > 1000){
			sprintf(text, " %0.0f fps  %0.2fms  �r.cz�sto�� = %0.2f[r/s]", fFps, 1000.0 / fFps, sr_czestosc);
			SetWindowText(okno, text); // wy�wietlenie aktualnej ilo�ci klatek/s w pasku okna			
		}
	}

	pMojObiekt->Symulacja(fDt);                    // symulacja w�asnego obiektu

	Ramka ramka;
	ramka.stan = pMojObiekt->Stan();               // stan w�asnego obiektu 

	// wyslanie komunikatu o stanie obiektu przypisanego do aplikacji (pMojObiekt):  
	int iRozmiar = uni_send->send((char*)&ramka, ip, sizeof(Ramka));
}

// *****************************************************************
// ****     Wszystko co trzeba zrobi� podczas zamykania aplikacji
// ****     poza grafik� 
void ZakonczenieInterakcji()
{
	fprintf(f, "Koniec interakcji WS\n");
	fclose(f);
}


// ************************************************************************
// ****     Obs�uga klawiszy s�u��cych do sterowania obiektami lub
// ****     widokami 
void KlawiszologiaSterowania(UINT kod_meldunku, WPARAM wParam, LPARAM lParam)
{

	switch (kod_meldunku)
	{

	case WM_LBUTTONDOWN: //reakcja na lewy przycisk myszki
	{
							 int x = LOWORD(lParam);
							 int y = HIWORD(lParam);

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
										pMojObiekt->ham = 1.0;       // stopie� hamowania (reszta zale�y od si�y docisku i wsp. tarcia)
										break;                       // 1.0 to maksymalny stopie� (np. zablokowanie k�)
					   }
					   case VK_UP:
					   {

									 pMojObiekt->F = 6.0;        // si�a pchaj�ca do przodu
									 break;
					   }
					   case VK_DOWN:
					   {
									   pMojObiekt->F = -2.0;
									   break;
					   }
					   case VK_LEFT:
					   {
									   if (SHIFTwcisniety) pMojObiekt->alfa = PI * 35 / 180;
									   else pMojObiekt->alfa = PI * 15 / 180;

									   break;
					   }
					   case VK_RIGHT:
					   {
										if (SHIFTwcisniety) pMojObiekt->alfa = -PI * 35 / 180;
										else pMojObiekt->alfa = -PI * 15 / 180;
										break;
					   }
					   case 'W':   // oddalenie widoku
					   {
									   //pol_kamery = pol_kamery - kierunek_kamery*0.3;
									   if (oddalenie > 0.5) oddalenie /= 1.3;
									   else oddalenie = 0;
									   break;
					   }
					   case 'S':   // przybli�enie widoku
					   {
									   //pol_kamery = pol_kamery + kierunek_kamery*0.3; 
									   if (oddalenie > 0) oddalenie *= 1.3;
									   else oddalenie = 0.5;
									   break;
					   }
					   case 'Q':   // widok z g�ry
					   {
									   widok_od_gory = 1 - widok_od_gory;
									   break;
					   }
					   case 'E':   // obr�t kamery ku g�rze (wzgl�dem lokalnej osi z)
					   {
									   kat_kam_z += PI * 5 / 180;
									   break;
					   }
					   case 'D':   // obr�t kamery ku do�owi (wzgl�dem lokalnej osi z)
					   {
									   kat_kam_z -= PI * 5 / 180;
									   break;
					   }
					   case 'A':   // w��czanie, wy��czanie trybu �ledzenia obiektu
					   {
									   sledzenie = 1 - sledzenie;
									   break;
					   }
					   case 'R':   // zwi�kszenie ogniskowej
					   {
									   zoom *= 1.2;
									   break;
					   }
					   case 'F':   // zmniejszenie ogniskowej
					   {
									   zoom /= 1.2;
									   break;
					   }
					   case VK_ESCAPE:
					   {
										 SendMessage(okno, WM_DESTROY, 0, 0);
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
