#include <stdio.h>
#include "kwaternion.h"

#define PI 3.1416

struct StanObiektu
{
  int iID;                  // identyfikator obiektu
	Wektor3 wPol;             // polozenie obiektu (�rodka geometrycznego obiektu) 
	kwaternion qOrient;       // orientacja (polozenie katowe)
	Wektor3 wV,wA;            // predkosc, przyspiesznie liniowe
  Wektor3 wV_kat, wA_kat;   // predkosc i przyspeszenie liniowe
	float masa_calk;          // masa ca�kowita (wa�na przy kolizjach)
	long pieniadze;
	int iID_wlasc;
	int czy_autonom;
};

class ObiektRuchomy
{
public:
	int iID;                  // identyfikator obiektu
	Wektor3 wPol;             // polozenie obiektu
	kwaternion qOrient;       // orientacja (polozenie katowe)
	Wektor3 wV,wA;            // predkosc, przyspiesznie liniowe
    Wektor3 wV_kat, wA_kat;   // predkosc i przyspeszenie liniowe

    long pieniadze;
    float ilosc_paliwa;

  float umiejetn_sadzenia,    // umiej�tno�� sadzenia drzew (1-pe�na, 0- brak)
        umiejetn_zb_paliwa,   // umiej�tno�� zbierania paliwa
        umiejetn_zb_monet;  

	float F,Fb;               // si�y dzia�aj�ce na obiekt: F - pchajaca do przodu, Fb - w prawo
	float ham;                // stopie� hamowania Fh_max = tarcie*Fy*ham
	float Fy;                 // si�a nacisku na podstaw� pojazdu - gdy obiekt styka si� z pod�o�em (od niej zale�y si�a hamowania)
	float alfa;               // kat skretu kol w radianach (w lewo - dodatni)
	float masa_wlasna;		  // masa w�asna obiektu (bez paliwa)	
	float masa_calk;          // masa ca�kowita (wa�na przy kolizjach)
	float dlugosc,szerokosc,wysokosc; // rozmiary w kierunku lokalnych osi x,y,z
	float promien;            // promien minimalnej sfery, w ktora wpisany jest obiekt
	float przeswit;           // wysoko�� na kt�rej znajduje si� podstawa obiektu
	float dl_przod;           // odleg�o�� od przedniej osi do przedniego zderzaka 
	float dl_tyl;             // odleg�o�� od tylniej osi do tylniego zderzaka
	long moment_wziecia;        // moment, w ktorym rozpoczeto branie
	long czas_oczekiwania;      // czas oczekiwania na finalizacj� procesu brania
	long nr_przedmiotu;         // numer przedmiotu do wziecia; -1 jesli zaden  

	int iID_kolid;            // identyfikator pojazdu z kt�rym nast�pi�a kolizja (-1  brak kolizji)
	Wektor3 wdV_kolid;        // poprawka pr�dko�ci pojazdu koliduj�cego
	int iID_wlasc; 
	bool czy_autonom;         // czy obiekt jest autonomiczny

public:
	ObiektRuchomy();          // konstruktor
	~ObiektRuchomy();
	void ZmienStan(StanObiektu stan);          // zmiana stanu obiektu
	StanObiektu Stan();        // metoda zwracajaca stan obiektu
    void Symulacja(float dt);  // symulacja ruchu obiektu w oparciu o biezacy stan, przylozone sily
	                           // oraz czas dzialania sil. Efektem symulacji jest nowy stan obiektu 
	void Rysuj();			   // odrysowanie obiektu					
};

enum typ {MONETA, BECZKA, DRZEWO, MUR };

enum podtyp {TOPOLA, SWIERK};

struct Przedmiot
{
	Wektor3 wPol;             // polozenie obiektu
	kwaternion qOrient;       // orientacja (polozenie katowe)

	int typ;
	int podtyp;
	long wartosc;             // w zal. od typu nomina� monety /ilosc paliwa, itd.
	float srednica;           // np. grubosc pnia u podstawy, srednica monety
	
	bool do_wziecia;          // czy przedmiot mozna wziac
  bool czy_ja_wzialem;      // czy przedmiot wziety przeze mnie
  bool czy_odnawialny;      // czy mo�e si� odnawia� w tym samym miejscu po pewnym czasie
  long czas_wziecia;        // czas wzi�cia (potrzebny do przywr�cenia)
};

class Teren
{
public:
    float **mapa;          // wysoko�ci naro�nik�w oraz �rodk�w p�l
    float ***d;            // warto�ci wyrazu wolnego z r�wnania p�aszczyzny dla ka�dego tr�jk�ta
    Wektor3 ***Norm;       // normalne do p�aszczyzn tr�jk�t�w
    float rozmiar_pola;    // dlugosc boku kwadratowego pola na mapie
    long lwierszy,lkolumn; // liczba wierszy i kolumn mapy (kwadrat�w na wysoko�� i szeroko��)     
    
    long liczba_przedmiotow;      // liczba przedmiot�w na planszy
    long liczba_przedmiotow_max;  // rozmiar tablicy przedmiot�w
    Przedmiot *p;          // tablica przedmiotow roznego typu
    
    Teren();    
    ~Teren();   
    float Wysokosc(float x,float z);      // okre�lanie wysoko�ci dla punktu o wsp. (x,z) 
    void Rysuj();	                      // odrysowywanie terenu   
    void PoczatekGrafiki();               // tworzenie listy wy�wietlania
};
