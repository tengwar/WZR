// Serwer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

unicast_net *uni_reciv;         // wsk do obiektu zajmujacego sie odbiorem komunikatow
unicast_net *uni_send;          //   -||-  wysylaniem komunikatow

struct Ramka                                    // podstawowa struktura komunikacyjna
{
	StanObiektu stan;                            // po³o¿enie, prêdkoœæ, itd.
	int typ;                                     // typ ramki - np. informacja o stanie, wiadomoœæ tekstowa, itd. 
	long czas_wyslania;
};

int _tmain(int argc, _TCHAR* argv[])
{
	// obiekty sieciowe typu unicast (z podaniem adresu IP wirtualnej grupy oraz numeru portu)
	uni_reciv = new unicast_net(10001);      // obiekt do odbioru ramek sieciowych
	uni_send = new unicast_net(10001);       // obiekt do wysy³ania ramek

	unicast_net *pmt_net = (unicast_net*)ptr;  // wskaŸnik do obiektu klasy unicast_net
	int rozmiar;                                 // liczba bajtów ramki otrzymanej z sieci
	Ramka ramka;
	unsigned long adres_nad = 0;

	while (true)
	{
		odbior_ramki();
		rozmiar = pmt_net->reciv((char*)&ramka, &adres_nad, sizeof(Ramka));   //TODO oczekiwanie na nadejœcie ramki

		przetworzenie_ramki();

		rozeslanie_ramki();
		int iRozmiar = uni_send->send((char*)&ramka, "224.12.12.25", sizeof(Ramka)); //TODO
	}

	return 0;
}

void odbior_ramki()
{

}

void przetworzenie_ramki()
{

}

void rozeslanie_ramki()
{

}

