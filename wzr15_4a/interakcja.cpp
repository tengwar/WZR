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
#include "grafika.h"

#include "siec.h"

FILE *f = fopen("wzr_log.txt","w");	// plik do zapisu informacji testowych

ObiektRuchomy *pMojObiekt;			// obiekt przypisany do tej aplikacji

Teren teren;
int iLiczbaCudzychOb = 0;
ObiektRuchomy *CudzeObiekty[1000];  // obiekty z innych aplikacji lub inne obiekty niz pCraft
int IndeksyOb[1000];                // tablica indeksow innych obiektow ulatwiajaca wyszukiwanie

float fDt;                          // sredni czas pomiedzy dwoma kolejnymi cyklami symulacji i wyswietlania
long czas_cyklu_WS,licznik_sym;     // zmienne pomocnicze potrzebne do obliczania fDt
float sr_czestosc;                  // srednia czestosc wysylania ramek w [ramkach/s] 
long czas_start = clock();          // czas od poczatku dzialania aplikacji  

multicast_net *multi_reciv;         // wsk do obiektu zajmujacego sie odbiorem komunikatow
multicast_net *multi_send;          //   -||-  wysylaniem komunikatow

HANDLE threadReciv;                 // uchwyt w�tku odbioru komunikat�w
extern HWND okno;       
int SHIFTwcisniety = 0;            


// Parametry widoku:
Wektor3 kierunek_kamery = Wektor3(10,-3,-11);   // kierunek patrzenia
Wektor3 pol_kamery = Wektor3(-5,3,10);          // po�o�enie kamery
Wektor3 pion_kamery = Wektor3(0,1,0);           // kierunek pionu kamery             
bool sledzenie = 0;                             // tryb �ledzenia obiektu przez kamer�
float oddalenie = 1.0;                          // oddalenie lub przybli�enie kamery
float zoom = 1.0;                               // zmiana k�ta widzenia
float kat_kam_z = 0;                            // obr�t kamery g�ra-d�
bool sterowanie_myszkowe = 0;                   // sterowanie pojazdem za pomoc� myszki
int kursor_x, kursor_y;                         // polo�enie kursora myszki w chwili w��czenia sterowania
char napis1[200], napis2[200];                  // napisy wy�wietlane w trybie graficznym 

int opoznienia = 0;                
bool podnoszenie_przedm = 1;        // czy mozna podnosic przedmioty
bool rejestracja_uczestnikow = 1;   // rejestracja trwa do momentu wzi�cia przedmiotu przez kt�regokolwiek uczestnika,
                                    // w przeciwnym razie trzeba by przesy�a� ca�y stan �rodowiska nowicjuszowi
float czas_odnowy_przedm = 90;      // czas w [s] po kt�rym wzi�te przedmioty odnawiaj� si�
bool czy_umiejetnosci = 1;          // czy zr�nicowanie umiej�tno�ci (dla ka�dego pojazdu losowane s� umiej�tno�ci
                                    // zbierania got�wki i paliwa)

extern float WyslaniePrzekazu(int ID_adresata, int typ_przekazu, float wartosc_przekazu);

enum typy_ramek {STAN_OBIEKTU, WZIECIE_PRZEDMIOTU, ODNOWIENIE_SIE_PRZEDMIOTU, KOLIZJA, PRZEKAZ, 
                 PROSBA_O_ZAMKNIECIE, NEGOCJACJE_HANDLOWE};

enum typy_przekazu {GOTOWKA, PALIWO};

struct Ramka
{  
	int typ_ramki;
	long czas_wyslania;
	int iID_adresata;	// nr ID adresata wiadomo�ci (pozostali uczestnicy powinni wiadomo�� zignorowa�)

	int nr_przedmiotu;	// nr przedmiotu, kt�ry zosta� wzi�ty lub odzyskany
	Wektor3 wdV_kolid;	// wektor pr�dko�ci wyj�ciowej po kolizji (uczestnik o wskazanym adresie powinien
						// przyj�� t� pr�dko��)
	
	int typ_przekazu;		// got�wka, paliwo
	float wartosc_przekazu;	// ilo�� got�wki lub paliwa
	int nr_druzyny;

	StanObiektu stan;
};


//******************************************
// Funkcja obs�ugi w�tku odbioru komunikat�w 
DWORD WINAPI WatekOdbioru(void *ptr)
{
	multicast_net *pmt_net=(multicast_net*)ptr;  // wska�nik do obiektu klasy multicast_net
	int rozmiar;                                 // liczba bajt�w ramki otrzymanej z sieci
	Ramka ramka;
	StanObiektu stan;

	while(1)
	{
		rozmiar = pmt_net->reciv((char*)&ramka,sizeof(Ramka));   // oczekiwanie na nadej�cie ramki 
		switch (ramka.typ_ramki) 
		{
		case STAN_OBIEKTU:           // podstawowy typ ramki informuj�cej o stanie obiektu              
			{
			stan = ramka.stan;
			//fprintf(f,"odebrano stan iID = %d, ID dla mojego obiektu = %d\n",stan.iID,pMojObiekt->iID);
			int niewlasny = 1;
			if ((stan.iID != pMojObiekt->iID))          // je�li to nie m�j w�asny obiekt
			{

				if ((rejestracja_uczestnikow)&&(IndeksyOb[stan.iID] == -1))        // nie ma jeszcze takiego obiektu w tablicy -> trzeba go stworzy�
				{
					CudzeObiekty[iLiczbaCudzychOb] = new ObiektRuchomy();   
					IndeksyOb[stan.iID] = iLiczbaCudzychOb;     // wpis do tablicy indeksowanej numerami ID
					// u�atwia wyszukiwanie, alternatyw� mo�e by� tabl. rozproszona                                                                                                           
					iLiczbaCudzychOb++;     
					CudzeObiekty[IndeksyOb[stan.iID]]->ZmienStan(stan);   // aktualizacja stanu obiektu obcego 			
				}        
				else if (IndeksyOb[stan.iID] != -1)
					CudzeObiekty[IndeksyOb[stan.iID]]->ZmienStan(stan);   // aktualizacja stanu obiektu obcego 	
				else                                                    
				{
					Ramka ramka;
					ramka.typ_ramki = PROSBA_O_ZAMKNIECIE;                // ��danie zamki�cia aplikacji
					ramka.iID_adresata = stan.iID;
					int iRozmiar = multi_send->send_delayed((char*)&ramka, sizeof(Ramka));
				}
			}
			break;
			}
		case WZIECIE_PRZEDMIOTU:            // ramka informuj�ca, �e kto� wzi�� przedmiot o podanym numerze
			{
			if ((ramka.nr_przedmiotu < teren.liczba_przedmiotow) && (stan.iID != pMojObiekt->iID))
			{
				teren.p[ramka.nr_przedmiotu].do_wziecia = 0;
				teren.p[ramka.nr_przedmiotu].czy_ja_wzialem = 0;
				rejestracja_uczestnikow = 0;
			}
			break;
			}
		case ODNOWIENIE_SIE_PRZEDMIOTU:       // ramka informujaca, �e przedmiot wcze�niej wzi�ty pojawi� si� znowu w tym samym miejscu
			{
			if (ramka.nr_przedmiotu < teren.liczba_przedmiotow)
				teren.p[ramka.nr_przedmiotu].do_wziecia = 1;
			break;
			}
		case KOLIZJA:                       // ramka informuj�ca o tym, �e obiekt uleg� kolizji
			{
			if (ramka.iID_adresata == pMojObiekt->iID)  // ID pojazdu, kt�ry uczestniczy� w kolizji zgadza si� z moim ID 
			{
				pMojObiekt->wdV_kolid = ramka.wdV_kolid; // przepisuje poprawk� w�asnej pr�dko�ci
				pMojObiekt->iID_kolid = pMojObiekt->iID; // ustawiam nr. kolidujacego jako w�asny na znak, �e powinienem poprawi� pr�dko��
			}
			break;
			}
		case PRZEKAZ:                       // ramka informuj�ca o przelewie pieni�nym lub przekazaniu towaru    
			{
			if (ramka.iID_adresata == pMojObiekt->iID)  // ID pojazdu, ktory otrzymal przelew zgadza si� z moim ID 
			{       
				if (ramka.typ_przekazu == GOTOWKA)
				pMojObiekt->pieniadze += ramka.wartosc_przekazu; 					
				else if (ramka.typ_przekazu == PALIWO)
				pMojObiekt->ilosc_paliwa += ramka.wartosc_przekazu;

				// nale�a�oby jeszcze przelew potwierdzi� (w UDP ramki mog� by� gubione!)
			}
			break;
			}
		case PROSBA_O_ZAMKNIECIE:           // ramka informuj�ca, �e powiniene� si� zamkn��
			{
			if (ramka.iID_adresata == pMojObiekt->iID)
			{   
				SendMessage(okno, WM_DESTROY, 0, 100);
			}
			break;
			}
		case NEGOCJACJE_HANDLOWE:
			{
			// ------------------------------------------------------------------------
			// --------------- MIEJSCE #1 NA NEGOCJACJE HANDLOWE  ---------------------
			// (szczeg�y na stronie w instrukcji do zadania)




			// ------------------------------------------------------------------------

			break;
			}

		} // switch po typach ramek
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

	for (long i=0; i<1000; i++)            // inicjacja indeksow obcych obiektow
		IndeksyOb[i] = -1;

	czas_cyklu_WS = clock();             // pomiar aktualnego czasu

	// obiekty sieciowe typu multicast (z podaniem adresu WZR oraz numeru portu)
	multi_reciv = new multicast_net("224.12.12.130", 10001);      // obiekt do odbioru ramek sieciowych
	multi_send = new multicast_net("224.12.12.130", 10001);       // obiekt do wysy�ania ramek

	if (opoznienia)
	{
		float srednie_opoznienie = 3 * (float)rand() / RAND_MAX, wariancja_opoznienia = 2;
		multi_send->PrepareDelay(srednie_opoznienie, wariancja_opoznienia);
	}

	// uruchomienie watku obslugujacego odbior komunikatow
	threadReciv = CreateThread(
		NULL,                        // no security attributes
		0,                           // use default stack size
		WatekOdbioru,                // thread function
		(void *)multi_reciv,         // argument to thread function
		0,                           // use default creation flags
		&dwThreadId);                // returns the thread identifier

}


// *****************************************************************
// ****    Wszystko co trzeba zrobi� w ka�dym cyklu dzia�ania 
// ****    aplikacji poza grafik� 
void Cykl_WS()
{
	licznik_sym++;  

	// obliczenie �redniego czasu pomi�dzy dwoma kolejnnymi symulacjami po to, by zachowa�  fizycznych 
	if (licznik_sym % 50 == 0)          // je�li licznik cykli przekroczy� pewn� warto��, to
	{                                   // nale�y na nowo obliczy� �redni czas cyklu fDt
		char text[200];
		long czas_pop = czas_cyklu_WS;
		czas_cyklu_WS = clock();
		float fFps = (50 * CLOCKS_PER_SEC) / (float)(czas_cyklu_WS - czas_pop);
		if (fFps != 0)
			fDt = 1.0 / fFps;
		else
			fDt = 1;

		sprintf(napis1, " %0.0f_fps, paliwo = %0.2f, gotowka = %d,", fFps, pMojObiekt->ilosc_paliwa, pMojObiekt->pieniadze);
		if (licznik_sym % 500 == 0)
			sprintf(napis2, "");
	}   

	pMojObiekt->Symulacja(fDt);                    // symulacja w�asnego obiektu


	if ((pMojObiekt->iID_kolid > -1) &&             // wykryto kolizj� - wysy�am specjaln� ramk�, by poinformowa� o tym drugiego uczestnika
		(pMojObiekt->iID_kolid != pMojObiekt->iID)) // oczywi�cie wtedy, gdy nie chodzi o m�j pojazd
	{
		Ramka ramka;
		ramka.typ_ramki = KOLIZJA;
		ramka.iID_adresata = pMojObiekt->iID_kolid;
		ramka.wdV_kolid = pMojObiekt->wdV_kolid;
		int iRozmiar = multi_send->send_delayed((char*)&ramka,sizeof(Ramka));
    
		char text[128];
		sprintf(napis2,"Kolizja z obiektem o ID = %d",pMojObiekt->iID_kolid);
		//SetWindowText(okno,text);

		pMojObiekt->iID_kolid = -1;
	}

	// wyslanie komunikatu o stanie obiektu przypisanego do aplikacji (pMojObiekt):    
	if (licznik_sym % 1 == 0)      
	{
		Ramka ramka;
		ramka.typ_ramki = STAN_OBIEKTU;
		ramka.stan = pMojObiekt->Stan();         // stan w�asnego obiektu 
		int iRozmiar = multi_send->send_delayed((char*)&ramka,sizeof(Ramka));
	} 

	long czas_akt = clock();

	// sprawdzam, czy nie najecha�em na monet� lub beczk� z paliwem. Je�li tak, to zdobywam pieni�dze lub paliwo oraz wysy�am innym uczestnikom 
	// informacj� o zabraniu beczki: (wcze�niej trzeba wcisn�� P) 
	for (long i=0;i<teren.liczba_przedmiotow;i++)
	{
		if ((teren.p[i].do_wziecia == 1)&&(podnoszenie_przedm)&&
			((teren.p[i].wPol - pMojObiekt->wPol + Wektor3(0,pMojObiekt->wPol.y - teren.p[i].wPol.y,0)).dlugosc() < pMojObiekt->promien))
		{
			long wartosc = teren.p[i].wartosc;
      
			if (teren.p[i].typ == MONETA)
			{
				if (czy_umiejetnosci)
					wartosc = (long)(float)wartosc*pMojObiekt->umiejetn_zb_monet;   
				pMojObiekt->pieniadze += wartosc;  
				sprintf(napis2,"Wziecie_gotowki_o_wartosci_ %d",wartosc);
			}
			else
			{
				if (czy_umiejetnosci)
					wartosc = (float)wartosc*pMojObiekt->umiejetn_zb_paliwa; 
				pMojObiekt->ilosc_paliwa += wartosc; 
				sprintf(napis2,"Wziecie_paliwa_w_ilosci_ %d",wartosc);
			}

			teren.p[i].do_wziecia = 0;
			teren.p[i].czy_ja_wzialem = 1;
			teren.p[i].czas_wziecia = clock();
			rejestracja_uczestnikow = 0;     // koniec rejestracji nowych uczestnik�w

			Ramka ramka;
			ramka.typ_ramki = WZIECIE_PRZEDMIOTU; 
			ramka.nr_przedmiotu = i;
			ramka.stan = pMojObiekt->Stan(); 
			int iRozmiar = multi_send->send_delayed((char*)&ramka,sizeof(Ramka));
		}
		else if ((teren.p[i].do_wziecia == 0)&&(teren.p[i].czy_ja_wzialem) &&
			(teren.p[i].czy_odnawialny) &&
			(czas_akt - teren.p[i].czas_wziecia >= czas_odnowy_przedm * CLOCKS_PER_SEC))
		{                              // je�li min�� pewnien okres czasu przedmiot mo�e zosta� przywr�cony
			teren.p[i].do_wziecia = 1;
			Ramka ramka;
			ramka.typ_ramki = ODNOWIENIE_SIE_PRZEDMIOTU; 
			ramka.nr_przedmiotu = i;
			int iRozmiar = multi_send->send_delayed((char*)&ramka, sizeof(Ramka));
		}
	} // for po przedmiotach

	// --------------------------------------------------------------------
	// --------------- MIEJSCE NA ALGORYTM STEROWANIA ---------------------
	// (dob�r si�y F w granicach (-2000 N, 4000 N), k�ta skr�tu k� alfa (-pi/4, pi/4) oraz
	// decyzji o hamowaniu ham w zale�no�ci od sytuacji)
  



	// --------------------------------------------------------------------


	// ------------------------------------------------------------------------
	// --------------- MIEJSCE #2 NA NEGOCJACJE HANDLOWE  ---------------------
	// (szczeg�y na stronie w instrukcji do zadania)




	// ------------------------------------------------------------------------
}

// *****************************************************************
// ****    Wszystko co trzeba zrobi� podczas zamykania aplikacji
// ****    poza grafik� 
void ZakonczenieInterakcji()
{
	fprintf(f, "Koniec interakcji\n");
	fclose(f);
}

// Funkcja wysylajaca ramke z przekazem, zwraca zrealizowan� warto�� przekazu
float WyslaniePrzekazu(int ID_adresata, int typ_przekazu, float wartosc_przekazu)
{
	Ramka ramka;
	ramka.typ_ramki = PRZEKAZ;
	ramka.iID_adresata = ID_adresata;
	ramka.typ_przekazu = typ_przekazu;
	ramka.wartosc_przekazu = wartosc_przekazu;

	// tutaj nale�a�oby uzyska� potwierdzenie przekazu zanim sumy zostan� odj�te
	if (typ_przekazu == GOTOWKA)
	{
		if (pMojObiekt->pieniadze < wartosc_przekazu)
			ramka.wartosc_przekazu = pMojObiekt->pieniadze;
		pMojObiekt->pieniadze -= ramka.wartosc_przekazu;
		sprintf(napis2,"Przelew_sumy_ %f _na_rzecz_ID_ %d",wartosc_przekazu,ID_adresata);
	}
	else if (typ_przekazu == PALIWO)
	{
		// odszukanie adresata, sprawdzenie czy jest odpowiednio blisko:
		int indeks_adresata = -1;
		for (int i=0;i<iLiczbaCudzychOb;i++)
			if (CudzeObiekty[i]->iID == ID_adresata) {indeks_adresata = i; break;}
		if ((CudzeObiekty[indeks_adresata]->wPol - pMojObiekt->wPol).dlugosc() > 
				CudzeObiekty[indeks_adresata]->dlugosc + pMojObiekt->dlugosc)
			ramka.wartosc_przekazu = 0;
		else
		{
			if (pMojObiekt->ilosc_paliwa < wartosc_przekazu)
			ramka.wartosc_przekazu = pMojObiekt->ilosc_paliwa;
			pMojObiekt->ilosc_paliwa -= ramka.wartosc_przekazu;
			sprintf(napis2,"Przekazanie_paliwa_w_ilo�ci_ %f _na_rzecz_ID_ %d",wartosc_przekazu,ID_adresata);
		}
	}

	if (ramka.wartosc_przekazu > 0)
	int iRozmiar = multi_send->send_delayed((char*)&ramka,sizeof(Ramka));  

	return ramka.wartosc_przekazu;
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
				pMojObiekt->F = 4100.0;        // si�a pchaj�ca do przodu
			break;
		}
		case WM_RBUTTONDOWN: //reakcja na prawy przycisk myszki
		{
			int x = LOWORD(lParam);
			int y = HIWORD(lParam);
			if (sterowanie_myszkowe)
				pMojObiekt->F = -2100.0;        // si�a pchaj�ca do tylu
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
				if (kat_skretu > 45)
					kat_skretu = 45;
				if (kat_skretu < -45)
					kat_skretu = -45;	
				pMojObiekt->alfa = PI * kat_skretu / 180;
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
					pMojObiekt->ham = 1.0;       // stopie� hamowania (reszta zale�y od si�y docisku i wsp. tarcia)
					break;                       // 1.0 to maksymalny stopie� (np. zablokowanie k�)
				}
				case VK_UP:
				{

					pMojObiekt->F = 4100.0;        // si�a pchaj�ca do przodu
					break;
				}
				case VK_DOWN:
				{
					pMojObiekt->F = -2100.0;        // sila pchajaca do tylu
					break;
				}
				case VK_LEFT:
				{
					if (SHIFTwcisniety)
						pMojObiekt->alfa = PI*35/180;
					else
						pMojObiekt->alfa = PI*15/180;

					break;
				}
				case VK_RIGHT:
				{
					if (SHIFTwcisniety)
						pMojObiekt->alfa = -PI*35/180;
					else
						pMojObiekt->alfa = -PI*15/180;
					break;
				}
				case 'W':   // przybli�enie widoku
				{
					//pol_kamery = pol_kamery - kierunek_kamery*0.3;
					if (oddalenie > 0.5)
						oddalenie /= 1.2;
					else
						oddalenie = 0;  
					break; 
				}     
				case 'S':   // oddalenie widoku
				{
					//pol_kamery = pol_kamery + kierunek_kamery*0.3; 
					if (oddalenie > 0)
						oddalenie *= 1.2;
					else
						oddalenie = 0.5;   
					break; 
				}    
				case 'Q':   // widok z g�ry
				{
					pol_kamery = Wektor3(0,100,0);
					kierunek_kamery = Wektor3(0,-1,0.01);
					pion_kamery = Wektor3(0,0,-1);
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
					break;
				}
				case 'Z':   // zoom - zmniejszenie k�ta widzenia
				{
					zoom /= 1.1;
					RECT rc;
					GetClientRect (okno, &rc);
					ZmianaRozmiaruOkna(rc.right - rc.left,rc.bottom-rc.top);
					break;
				}
				case 'X':   // zoom - zwi�kszenie k�ta widzenia
				{
					zoom *= 1.1;
					RECT rc;
					GetClientRect (okno, &rc);
					ZmianaRozmiaruOkna(rc.right - rc.left,rc.bottom-rc.top);
					break;
				}
				case 'P':   // podnoszenie przedmiot�w
				{
					//Wektor3 w_przod = pMojObiekt->qOrient.obroc_wektor(Wektor3(1,0,0));
					//+ Wector3(0,pMojObiekt->wPol.y - teren.p[i].wPol.y,0)
					podnoszenie_przedm = 1 - podnoszenie_przedm;
					break;
				}
				case 'F':  // przekazanie 10 kg paliwa pojazdowi, kt�ry znajduje si� najbli�ej
				{
					float min_odleglosc = 1e10;
					int indeks_min = -1;
					for (int i=0; i<iLiczbaCudzychOb; i++)
					{
						if (min_odleglosc > (CudzeObiekty[i]->wPol - pMojObiekt->wPol).dlugosc() )
						{
							min_odleglosc = (CudzeObiekty[i]->wPol - pMojObiekt->wPol).dlugosc();
							indeks_min = i;
						}
					}

					float ilosc_p =  0;
					if (indeks_min > -1)
						ilosc_p = WyslaniePrzekazu(CudzeObiekty[indeks_min]->iID, PALIWO, 10);
          
					if (ilosc_p == 0) 
						MessageBox(okno,"Paliwa nie da�o si� przekaza�, bo by� mo�e najbli�szy obiekt ruchomy znajduje si� zbyt daleko.",
							"Nie da�o si� przekaza� paliwa!",MB_OK);
					break;
				}
				case 'G':  // przekazanie 100 jednostek gotowki pojazdowi, kt�ry znajduje si� najbli�ej
				{
					float min_odleglosc = 1e10;
					int indeks_min = -1;
					for (int i=0;i<iLiczbaCudzychOb;i++)
					{
						if (min_odleglosc > (CudzeObiekty[i]->wPol - pMojObiekt->wPol).dlugosc() )
						{
							min_odleglosc = (CudzeObiekty[i]->wPol - pMojObiekt->wPol).dlugosc();
							indeks_min = i;
						}
					}

					float ilosc_p =  0;
					if (indeks_min > -1)
						ilosc_p = WyslaniePrzekazu(CudzeObiekty[indeks_min]->iID, GOTOWKA, 100);
          
					if (ilosc_p == 0) 
						MessageBox(okno,"Got�wki nie da�o si� przekaza�, bo by� mo�e najbli�szy obiekt ruchomy znajduje si� zbyt daleko.",
							"Nie da�o si� przekaza� got�wki!",MB_OK);
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
