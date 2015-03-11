#include <stdio.h>
#include "kwaternion.h"

#define PI 3.1416

struct StanObiektu
{
  int iID;                  // identyfikator obiektu
	Wektor3 wPol;             // polozenie obiektu (�rodka geometrycznego obiektu) 
	kwaternion qOrient;       // orientacja (polozenie katowe)
	Wektor3 wV,wA;            // predkosc, przyspiesznie liniowe
  Wektor3 wV_kat, wA_kat;   // predkosc i przyspieszenie liniowe
};

// Klasa opisuj�ca obiekty ruchome
class ObiektRuchomy
{
public:
	int iID;                  // identyfikator obiektu
	Wektor3 wPol;             // polozenie obiektu
	kwaternion qOrient;       // orientacja (polozenie katowe) 
	Wektor3 wV,wA;            // predkosc, przyspiesznie liniowe
  Wektor3 wV_kat, wA_kat;   // predkosc i przyspeszenie liniowe

	float F,Fb;               // si�y dzia�aj�ce na obiekt: F - pchajaca do przodu, Fb - w prawo
	float ham;                // stopie� hamowania Fh_max = tarcie*Fy*ham
	float Fy;                 // si�a nacisku na podstaw� pojazdu - gdy obiekt styka si� z pod�o�em (od niej zale�y si�a hamowania)
	float alfa;               // kat skretu kol w radianach (w lewo - dodatni)
	float m;				  // masa obiektu	
	float dlugosc,szerokosc,wysokosc; // rozmiary w kierunku lokalnych osi x,y,z
	float przeswit;           // wysoko�� na kt�rej znajduje si� podstawa obiektu
	float dl_przod;           // odleg�o�� od przedniej osi do przedniego zderzaka 
	float dl_tyl;             // odleg�o�� od tylniej osi do tylniego zderzaka

public:
	ObiektRuchomy();          // konstruktor
	~ObiektRuchomy();
	void ZmienStan(StanObiektu stan);          // zmiana stanu obiektu
	StanObiektu Stan();        // metoda zwracajaca stan obiektu
    void Symulacja(float dt);  // symulacja ruchu obiektu w oparciu o biezacy stan, przylozone sily
	                           // oraz czas dzialania sil. Efektem symulacji jest nowy stan obiektu 
	void Rysuj();			   // odrysowanie obiektu					
};

// Klasa opisuj�ca teren, po kt�rym poruszaj� si� obiekty
class Teren
{
public:
    float **mapa;          // wysoko�ci naro�nik�w oraz �rodk�w p�l
    float ***d;            // warto�ci wyrazu wolnego z r�wnania p�aszczyzny dla ka�dego tr�jk�ta
    Wektor3 ***Norm;       // normalne do p�aszczyzn tr�jk�t�w
    float rozmiar_pola;    // dlugosc boku kwadratowego pola na mapie
    long lwierszy,lkolumn; // liczba wierszy i kolumn mapy (kwadrat�w na wysoko�� i szeroko��)     
    Teren();    
    ~Teren();   
    float Wysokosc(float x,float z);      // okre�lanie wysoko�ci dla punktu o wsp. (x,z) 
    void Rysuj();	                      // odrysowywanie terenu   
    void PoczatekGrafiki();               // tworzenie listy wy�wietlania
};
