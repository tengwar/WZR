/*********************************************************************
Symulacja obiekt�w fizycznych ruchomych np. samochody, statki, roboty, itd. 
+ obs�uga obiekt�w statycznych np. teren.
**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include <gl\gl.h>
#include <gl\glu.h>
#include "obiekty.h"
#include "grafika.h"


//#include "wektor.h"
extern FILE *f;
extern Teren teren;
extern int iLiczbaCudzychOb;
extern ObiektRuchomy *CudzeObiekty[1000]; 


ObiektRuchomy::ObiektRuchomy()             // konstruktor                   
{

  //iID = (unsigned int)(clock() % 1000);  // identyfikator obiektu
  iID = (unsigned int)(rand() % 1000);  // identyfikator obiektu
  fprintf(f,"Nowy obiekt: iID = %d\n",iID);
  iID_wlasc = iID;           // identyfikator w�a�ciciela obiektu
  czy_autonom = 0;

  pieniadze = 1000;    // np. z�otych
  ilosc_paliwa = 10.0;   // np. kilogram�w paliwa 

  F = Fb = alfa  = 0;	      // si�y dzia�aj�ce na obiekt 
  ham = 0;			      // stopie� hamowania
  masa_wlasna = 1200.0;     // masa w�asna obiektu [kg] (bez paliwa)
  masa_calk = masa_wlasna + ilosc_paliwa;  // masa ca�kowita
  Fy = masa_wlasna*9.81;    // si�a nacisku na podstaw� obiektu (na ko�a pojazdu)
  dlugosc = 7.0;
  szerokosc = 2.8;
  wysokosc = 1.0;
  przeswit = 0.0;           // wysoko�� na kt�rej znajduje si� podstawa obiektu
  dl_przod = 1.0;           // odleg�o�� od przedniej osi do przedniego zderzaka 
  dl_tyl = 0.2;             // odleg�o�� od tylniej osi do tylniego zderzaka

  iID_kolid = -1;           // na razie brak kolizji 

  wPol.y = przeswit+wysokosc/2 + 10;
  promien = sqrt(dlugosc*dlugosc + szerokosc*szerokosc + wysokosc*wysokosc)/2/1.15;
  //wV_kat = Wektor3(0,1,0)*40;  // pocz�tkowa pr�dko�� k�towa (w celach testowych)

  moment_wziecia = 0;            // czas ostatniego wziecia przedmiotu
  czas_oczekiwania = 1000000000; // 
  nr_przedmiotu = -1;

  // obr�t obiektu o k�t 30 stopni wzgl�dem osi y:
  kwaternion qObr = AsixToQuat(Wektor3(0,1,0),0.1*PI/180.0);
  qOrient = qObr*qOrient;
  
  // losowanie umiej�tno�ci tak by nie by�o bardzo s�abych i bardzo silnych:
  umiejetn_sadzenia = (float)rand()/RAND_MAX;
  umiejetn_zb_paliwa = (float)rand()/RAND_MAX;
  umiejetn_zb_monet = (float)rand()/RAND_MAX;
  float suma_um = umiejetn_sadzenia + umiejetn_zb_paliwa + umiejetn_zb_monet;
  float suma_um_los = 0.7 + 0.8*(float)rand()/RAND_MAX;    // losuje umiejetno�� sumaryczn�
  umiejetn_sadzenia *= suma_um_los/suma_um;
  umiejetn_zb_paliwa *= suma_um_los/suma_um;
  umiejetn_zb_monet *= suma_um_los/suma_um;

}

ObiektRuchomy::~ObiektRuchomy()            // destruktor
{
}

void ObiektRuchomy::ZmienStan(StanObiektu stan)  // przepisanie podanego stanu 
{                                                // w przypadku obiekt�w, kt�re nie s� symulowane
  this->iID = stan.iID;                        

  this->wPol = stan.wPol;
  this->qOrient = stan.qOrient;
  this->wV = stan.wV;
  this->wA = stan.wA;
  this->wV_kat = stan.wV_kat;
  this->wA_kat = stan.wA_kat;

  this->masa_calk = stan.masa_calk;
  this->pieniadze = stan.pieniadze;
  this->iID_wlasc = stan.iID_wlasc;
  this->czy_autonom = stan.czy_autonom;
}

StanObiektu ObiektRuchomy::Stan()                // metoda zwracaj�ca stan obiektu ��cznie z iID
{
  StanObiektu stan;

  stan.iID = iID;
  stan.qOrient = qOrient;
  stan.wA = wA;
  stan.wA_kat = wA_kat;
  stan.wPol = wPol;
  stan.wV = wV;
  stan.wV_kat = wV_kat;

  stan.masa_calk = masa_calk;
  stan.pieniadze = pieniadze;
  stan.iID_wlasc = iID_wlasc;
  stan.czy_autonom = czy_autonom;
  return stan;
}

void ObiektRuchomy::Symulacja(float dt)          // obliczenie nowego stanu na podstawie dotychczasowego,
{                                                // dzia�aj�cych si� i czasu, jaki up�yn�� od ostatniej symulacji

  if (dt == 0) return;

  float tarcie = 0.6;            // wsp�czynnik tarcia obiektu o pod�o�e 
  float tarcie_obr = tarcie;     // tarcie obrotowe (w szczeg�lnych przypadkach mo�e by� inne ni� liniowe)
  float tarcie_toczne = 0.05;    // wsp�czynnik tarcia tocznego
  float sprezystosc = 0.5;       // wsp�czynnik spr�ysto�ci (0-brak spr�ysto�ci, 1-doskona�a spr�ysto��) 
  float g = 9.81;                // przyspieszenie grawitacyjne
  float m = masa_wlasna + ilosc_paliwa;   // masa calkowita


  // obracam uk�ad wsp�rz�dnych lokalnych wed�ug kwaterniona orientacji:
  Wektor3 w_przod = qOrient.obroc_wektor(Wektor3(1,0,0)); // na razie o� obiektu pokrywa si� z osi� x globalnego uk�adu wsp�rz�dnych (lokalna o� x)
  Wektor3 w_gora = qOrient.obroc_wektor(Wektor3(0,1,0));  // wektor skierowany pionowo w g�r� od podstawy obiektu (lokalna o� y)
  Wektor3 w_prawo = qOrient.obroc_wektor(Wektor3(0,0,1)); // wektor skierowany w prawo (lokalna o� z)

  fprintf(f,"w_przod = (%f, %f, %f)\n",w_przod.x,w_przod.y,w_przod.z);
  fprintf(f,"w_gora = (%f, %f, %f)\n",w_gora.x,w_gora.y,w_gora.z);
  fprintf(f,"w_prawo = (%f, %f, %f)\n",w_prawo.x,w_prawo.y,w_prawo.z);

  fprintf(f,"|w_przod|=%f,|w_gora|=%f,|w_prawo|=%f\n",w_przod.dlugosc(),w_gora.dlugosc(),w_prawo.dlugosc()  );
  fprintf(f,"ilo skalar = %f,%f,%f\n",w_przod^w_prawo,w_przod^w_gora,w_gora^w_prawo  );
  //fprintf(f,"w_przod = (%f, %f, %f) w_gora = (%f, %f, %f) w_prawo = (%f, %f, %f)\n",
  //           w_przod.x,w_przod.y,w_przod.z,w_gora.x,w_gora.y,w_gora.z,w_prawo.x,w_prawo.y,w_prawo.z);


  // rzutujemy wV na sk�adow� w kierunku przodu i pozosta�e 2 sk�adowe
  // sk�adowa w bok jest zmniejszana przez si�� tarcia, sk�adowa do przodu
  // przez si�� tarcia tocznego
  Wektor3 wV_wprzod = w_przod*(wV^w_przod),
    wV_wprawo = w_prawo*(wV^w_prawo),
    wV_wgore = w_gora*(wV^w_gora); 

  // rzutujemy pr�dko�� k�tow� wV_kat na sk�adow� w kierunku przodu i pozosta�e 2 sk�adowe
  Wektor3 wV_kat_wprzod = w_przod*(wV_kat^w_przod),
    wV_kat_wprawo = w_prawo*(wV_kat^w_prawo),
    wV_kat_wgore = w_gora*(wV_kat^w_gora);         


  // ograniczenia 
  if (F > 4000) F = 4000;
  if (F < -2000) F = -2000;
  if (alfa > PI*45.0/180) alfa = PI*45.0/180 ;
  if (alfa < -PI*45.0/180) alfa = -PI*45.0/180 ;

  // obliczam promien skr�tu pojazdu na podstawie k�ta skr�tu k�, a nast�pnie na podstawie promienia skr�tu
  // obliczam pr�dko�� k�tow� (UPROSZCZENIE! pomijam przyspieszenie k�towe oraz w�a�ciw� trajektori� ruchu)
  if (Fy > 0)
  {
    float V_kat_skret = 0;
    if (alfa != 0)
    {   
      float Rs = sqrt(dlugosc*dlugosc/4 + (fabs(dlugosc/tan(alfa)) + szerokosc/2)*(fabs(dlugosc/tan(alfa)) + szerokosc/2));
      V_kat_skret = wV_wprzod.dlugosc()*(1.0/Rs);
    }	
    Wektor3 wV_kat_skret = w_gora*V_kat_skret*(alfa > 0 ? 1 : -1);
    Wektor3 wV_kat_wgore2 = wV_kat_wgore + wV_kat_skret;
    if (wV_kat_wgore2.dlugosc() <= wV_kat_wgore.dlugosc()) // skr�t przeciwdzia�a obrotowi
    {
      if (wV_kat_wgore2.dlugosc() > V_kat_skret)
        wV_kat_wgore = wV_kat_wgore2; 
      else  
        wV_kat_wgore = wV_kat_skret;
    }    
    else    
    {
      if (wV_kat_wgore.dlugosc() < V_kat_skret)
        wV_kat_wgore = wV_kat_skret;

    }

    // tarcie zmniejsza pr�dko�� obrotow� (UPROSZCZENIE! zamiast masy winienem wykorzysta� moment bezw�adno�ci)     
    float V_kat_tarcie = Fy*tarcie_obr*dt/m/1.0;      // zmiana pr. k�towej spowodowana tarciem
    float V_kat_wgore = wV_kat_wgore.dlugosc() - V_kat_tarcie;
    if (V_kat_wgore < V_kat_skret) V_kat_wgore = V_kat_skret;        // tarcie nie mo�e spowodowa� zmiany zwrotu wektora pr. k�towej
    wV_kat_wgore = wV_kat_wgore.znorm()*V_kat_wgore;                     
  }    




  Fy = m*g*w_gora.y;                      // si�a docisku do pod�o�a 
  if (Fy < 0 ) Fy = 0;
  // ... trzeba j� jeszcze uzale�ni� od tego, czy obiekt styka si� z pod�o�em!
  float Fh = Fy*tarcie*ham;                  // si�a hamowania (UP: bez uwzgl�dnienia po�lizgu)

  float V_wprzod = wV_wprzod.dlugosc();// - dt*Fh/m - dt*tarcie_toczne*Fy/m;
  if (V_wprzod < 0) V_wprzod = 0;

  float V_wprawo = wV_wprawo.dlugosc();// - dt*tarcie*Fy/m;
  if (V_wprawo < 0) V_wprawo = 0;


  // wjazd lub zjazd: 
  //wPol.y = teren.Wysokosc(wPol.x,wPol.z);   // najprostsze rozwi�zanie - obiekt zmienia wysoko�� bez zmiany orientacji

  // 1. gdy wjazd na wkl�s�o��: wyznaczam wysoko�ci terenu pod naro�nikami obiektu (ko�ami), 
  // sprawdzam kt�ra tr�jka
  // naro�nik�w odpowiada najni�ej po�o�onemu �rodkowi ci�ko�ci, gdy przylega do terenu
  // wyznaczam pr�dko�� podbicia (wznoszenia �rodka pojazdu spowodowanego wkl�s�o�ci�) 
  // oraz pr�dko�� k�tow�
  // 2. gdy wjazd na wypuk�o�� to si�a ci�ko�ci wywo�uje obr�t przy du�ej pr�dko�ci liniowej

  // punkty zaczepienia k� (na wysoko�ci pod�ogi pojazdu):
  Wektor3 P = wPol + w_przod*(dlugosc/2-dl_przod) - w_prawo*szerokosc/2 - w_gora*wysokosc/2,
    Q = wPol + w_przod*(dlugosc/2-dl_przod) + w_prawo*szerokosc/2 - w_gora*wysokosc/2,
    R = wPol + w_przod*(-dlugosc/2+dl_tyl) - w_prawo*szerokosc/2 - w_gora*wysokosc/2,
    S = wPol + w_przod*(-dlugosc/2+dl_tyl) + w_prawo*szerokosc/2 - w_gora*wysokosc/2;

  // pionowe rzuty punkt�w zacz. k� pojazdu na powierzchni� terenu:  
  Wektor3 Pt = P, Qt = Q, Rt = R, St = S;
  Pt.y = teren.Wysokosc(P.x,P.z); Qt.y = teren.Wysokosc(Q.x,Q.z);  
  Rt.y = teren.Wysokosc(R.x,R.z); St.y = teren.Wysokosc(S.x,S.z);   
  Wektor3 normPQR = normalna(Pt,Rt,Qt), normPRS = normalna(Pt,Rt,St), normPQS = normalna(Pt,St,Qt),
    normQRS = normalna(Qt,Rt,St);   // normalne do p�aszczyzn wyznaczonych przez tr�jk�ty

  fprintf(f,"P.y = %f, Pt.y = %f, Q.y = %f, Qt.y = %f, R.y = %f, Rt.y = %f, S.y = %f, St.y = %f\n",
    P.y, Pt.y, Q.y, Qt.y, R.y,Rt.y, S.y, St.y);

  float sryPQR = ((Qt^normPQR) - normPQR.x*wPol.x - normPQR.z*wPol.z)/normPQR.y, // wys. �rodka pojazdu
    sryPRS = ((Pt^normPRS) - normPRS.x*wPol.x - normPRS.z*wPol.z)/normPRS.y, // po najechaniu na skarp� 
    sryPQS = ((Pt^normPQS) - normPQS.x*wPol.x - normPQS.z*wPol.z)/normPQS.y, // dla 4 tr�jek k�
    sryQRS = ((Qt^normQRS) - normQRS.x*wPol.x - normQRS.z*wPol.z)/normQRS.y;
  float sry = sryPQR; Wektor3 norm = normPQR;
  if (sry > sryPRS) {sry = sryPRS; norm = normPRS;}  
  if (sry > sryPQS) {sry = sryPQS; norm = normPQS;}
  if (sry > sryQRS) {sry = sryQRS; norm = normQRS;}  // wyb�r tr�jk�ta o �rodku najni�ej po�o�onym    




  Wektor3 wV_kat_wpoziomie = Wektor3(0,0,0);
  // jesli kt�re� z k� jest poni�ej powierzchni terenu
  if ((P.y <= Pt.y + wysokosc/2+przeswit)||(Q.y <= Qt.y  + wysokosc/2+przeswit)||
    (R.y <= Rt.y  + wysokosc/2+przeswit)||(S.y <= St.y  + wysokosc/2+przeswit))
  {   
    // obliczam powsta�� pr�dko�� k�tow� w lokalnym uk�adzie wsp�rz�dnych:      
    Wektor3 wobrot = -norm.znorm()*w_gora*0.6;  
    wV_kat_wpoziomie = wobrot/dt; 
  }    

  Wektor3 wAg = Wektor3(0,-1,0)*g;    // przyspieszenie grawitacyjne

  // jesli wiecej niz 2 kola sa na ziemi, to przyspieszenie grawitacyjne jest rownowazone przez opor gruntu:
  if ((P.y <= Pt.y + wysokosc/2+przeswit)+(Q.y <= Qt.y  + wysokosc/2+przeswit)+
    (R.y <= Rt.y  + wysokosc/2+przeswit)+(S.y <= St.y  + wysokosc/2+przeswit) > 2)
  {	
    wAg = wAg + 
      w_gora*(w_gora^wAg)*-1; //przyspieszenie wynikaj�ce z si�y oporu gruntu
  }
  else   // w przeciwnym wypadku brak sily docisku 
    Fy = 0;





  // sk�adam z powrotem wektor pr�dko�ci k�towej: 
  //wV_kat = wV_kat_wgore + wV_kat_wprawo + wV_kat_wprzod;  
  wV_kat = wV_kat_wgore + wV_kat_wpoziomie;


  float h = sry+wysokosc/2+przeswit - wPol.y;  // r�nica wysoko�ci jak� trzeba pokona�  
  float V_podbicia = 0;
  if ((h > 0)&&(wV.y <= 0.01))
    V_podbicia = 0.5*sqrt(2*g*h);  // pr�dko�� spowodowana podbiciem pojazdu przy wje�d�aniu na skarp� 
  if (h > 0) wPol.y = sry+wysokosc/2+przeswit;  

  // lub  w przypadku zag��bienia si� 
  //fprintf(f,"sry = %f, wPol.y = %f, dt = %f\n",sry,wPol.y,dt);  
  //fprintf(f,"normPQR.y = %f, normPRS.y = %f, normPQS.y = %f, normQRS.y = %f\n",normPQR.y,normPRS.y,normPQS.y,normQRS.y); 

  Wektor3 dwPol = wV*dt;//wA*dt*dt/2; // czynnik bardzo ma�y - im wi�ksza cz�stotliwo�� symulacji, tym mniejsze znaczenie 
  wPol = wPol + dwPol;  

  // Sprawdzenie czy obiekt mo�e si� przemie�ci� w zadane miejsce: Je�li nie, to 
  // przemieszczam obiekt do miejsca zetkni�cia, wyznaczam nowe wektory pr�dko�ci
  // i pr�dko�ci k�towej, a nast�pne obliczam nowe po�o�enie na podstawie nowych
  // pr�dko�ci i pozosta�ego czasu. Wszystko powtarzam w p�tli (pojazd znowu mo�e 
  // wjecha� na przeszkod�). Problem z zaokr�glonymi przeszkodami - konieczne 
  // wyznaczenie minimalnego kroku.


  Wektor3 wV_pop = wV;  

  // sk�adam pr�dko�ci w r�nych kierunkach oraz efekt przyspieszenia w jeden wektor:    (problem z przyspieszeniem od si�y tarcia -> to przyspieszenie 
  //      mo�e dzia�a� kr�cej ni� dt -> trzeba to jako� uwzgl�dni�, inaczej poazd b�dzie w�ykowa�)
  wV = wV_wprzod.znorm()*V_wprzod + wV_wprawo.znorm()*V_wprawo + wV_wgore + 
    Wektor3(0,1,0)*V_podbicia + wA*dt;
  // usuwam te sk�adowe wektora pr�dko�ci w kt�rych kierunku jazda nie jest mo�liwa z powodu
  // przesk�d:
  // np. je�li pojazd styka si� 3 ko�ami z nawierzchni� lub dwoma ko�ami i �rodkiem ci�ko�ci to
  // nie mo�e mie� pr�dko�ci w d� pod�ogi
  if ((P.y <= Pt.y  + wysokosc/2+przeswit)||(Q.y <= Qt.y  + wysokosc/2+przeswit)||  
    (R.y <= Rt.y  + wysokosc/2+przeswit)||(S.y <= St.y  + wysokosc/2+przeswit))    // je�li pojazd styka si� co najm. jednym ko�em
  {
    Wektor3 dwV = wV_wgore + w_gora*(wA^w_gora)*dt;
    if ((w_gora.znorm() - dwV.znorm()).dlugosc() > 1 )  // je�li wektor skierowany w d� pod�ogi
      wV = wV - dwV;
  }

  /*fprintf(f," |wV_wprzod| %f -> %f, |wV_wprawo| %f -> %f, |wV_wgore| %f -> %f |wV| %f -> %f\n",
  wV_wprzod.dlugosc(), (wV_wprzod.znorm()*V_wprzod).dlugosc(), 
  wV_wprawo.dlugosc(), (wV_wprawo.znorm()*V_wprawo).dlugosc(),
  wV_wgore.dlugosc(), (wV_wgore.znorm()*wV_wgore.dlugosc()).dlugosc(),
  wV_pop.dlugosc(), wV.dlugosc()); */

  // sk�adam przyspieszenia liniowe od si� nap�dzaj�cych i od si� oporu: 
  wA = (w_przod*F + w_prawo*Fb)/m*(Fy>0)*(ilosc_paliwa>0)  // od si� nap�dzaj�cych
    - wV_wprzod.znorm()*(Fh/m + tarcie_toczne*Fy/m)*(V_wprzod>0.01) // od hamowania i tarcia tocznego (w kierunku ruchu)
    - wV_wprawo.znorm()*tarcie*Fy/m*(V_wprawo>0.01)    // od tarcia w kierunku prost. do kier. ruchu
    + wAg;           // od grawitacji


  // utrata paliwa:
  ilosc_paliwa -= (fabs(F) + fabs(Fb))*(Fy>0)*dt/20000;      
  if (ilosc_paliwa < 0 ) ilosc_paliwa = 0;     
  masa_calk = masa_wlasna + ilosc_paliwa;


  // obliczenie nowej orientacji:
  Wektor3 w_obrot = wV_kat*dt;// + wA_kat*dt*dt/2;    
  kwaternion q_obrot = AsixToQuat(w_obrot.znorm(),w_obrot.dlugosc());
  //fprintf(f,"w_obrot = (x=%f, y=%f, z=%f) \n",w_obrot.x, w_obrot.y, w_obrot.z );
  //fprintf(f,"q_obrot = (w=%f, x=%f, y=%f, z=%f) \n",q_obrot.w, q_obrot.x, q_obrot.y, q_obrot.z );
  qOrient = q_obrot*qOrient; 	
  fprintf(f,"Pol = (%f, %f, %f) V = (%f, %f, %f) A = (%f, %f, %f) V_kat = (%f, %f, %f) ID = %d\n",
    wPol.x,wPol.y,wPol.z,wV.x,wV.y,wV.z,wA.x,wA.y,wA.z,wV_kat.x,wV_kat.y,wV_kat.z,iID);

  // wykrywanie kolizji z drzewami:
  for (long i=0;i<teren.liczba_przedmiotow;i++)    
    if (teren.p[i].typ == DRZEWO)
    {
      // bardzo duze uproszczenie -> traktuje pojazd jako kul�
      Wektor3 wPolDrz = teren.p[i].wPol; 
      wPolDrz.y = (wPolDrz.y + teren.p[i].wartosc > wPol.y ? wPol.y : wPolDrz.y + teren.p[i].wartosc);                        
      if  ((wPolDrz - wPol).dlugosc() < promien + teren.p[i].srednica/2)  // jesli kolizja
      {             
        // od wektora predkosci odejmujemy jego rzut na kierunek od punktu styku do osi drzewa:
        // jesli pojazd juz wjechal w drzewo, to nieco zwiekszamy poprawke     
        // punkt styku znajdujemy laczac krawedz pojazdu z osia drzewa odcinkiem
        // do obu prostopadlym      
        Wektor3 dP = (wPolDrz - wPol).znorm();   // wektor, w ktorego kierunku ruch jest niemozliwy
        float k = wV^dP;
        if (k > 0)     // jesli jest skladowa predkosci w strone drzewa
        {
          wV = wV - dP*k*(1 + sprezystosc);  // odjecie skladowej + odbicie sprezyste 
          //wV_kat = 
        }  
      }                              
    }	

    // kolizje z innymi obiektami
    if (iID_kolid == iID) // kto� o numerze iID_kolid wykry� kolizj� z naszym pojazdem i poinformowa� nas o tym

    {
      fprintf(f,"ktos wykryl kolizje - modyf. predkosci\n",iID_kolid);
      wV = wV + wdV_kolid;   // modyfikuje pr�dko�� o wektor obliczony od drugiego (�yczliwego) uczestnika
      iID_kolid = -1;

    }
    else      
    {
      for (long i=0;i<iLiczbaCudzychOb;i++)
      {
        ObiektRuchomy *inny = CudzeObiekty[i];      

        if ((wPol - inny->wPol).dlugosc() < 2*promien)  // je�li kolizja (zak�adam, �e drugi obiekt ma taki sam promie�
        {
          // zderzenie takie jak w symulacji kul 
          Wektor3 norm_pl_st = (wPol - inny->wPol).znorm();    // normalna do p�aszczyzny stycznej - kierunek odbicia
          float m1 = masa_calk, m2 = inny->masa_calk;          // masy obiekt�w
          float W1 = wV^norm_pl_st, W2 = inny->wV^norm_pl_st;  // wartosci pr�dko�ci
          if (W2>W1)      // je�li obiekty si� przybli�aj�
          {

            float Wns = (m1*W1 + m2*W2)/(m1+m2);        // pr�dko�� po zderzeniu ca�kowicie niespr�ystym
            float W1s = ((m1-m2)*W1 + 2*m2*W2)/(m1+m2), // pr�dko�� po zderzeniu ca�kowicie spr�ystym
              W2s = ((m2-m1)*W2 + 2*m1*W1)/(m1+m2);
            float W1sp = Wns +(W1s-Wns)*sprezystosc;    // pr�dko�� po zderzeniu spr�ysto-plastycznym
            float W2sp = Wns +(W2s-Wns)*sprezystosc;

            wV = wV + norm_pl_st*(W1sp-W1);    // poprawka pr�dko�ci (zak�adam, �e inny w przypadku drugiego obiektu zrobi to jego w�asny symulator) 
            iID_kolid = inny->iID;
            wdV_kolid = norm_pl_st*(W2sp-W2);
            fprintf(f,"wykryto i zreal. kolizje z %d W1=%f,W2=%f,W1s=%f,W2s=%f,m1=%f,m2=%f\n",iID_kolid,W1,W2,W1s,W2s,m1,m2); 
          }
          //if (fabs(W2 - W1)*dt < (wPol - inny->wPol).dlugosc() < 2*promien) wV = wV + norm_pl_st*(W1sp-W1)*2;
        }
      }
    } // do else
}

void ObiektRuchomy::Rysuj()
{
  glPushMatrix();


  glTranslatef(wPol.x,wPol.y+przeswit,wPol.z);


  kwaternion k = qOrient.AsixAngle();
  //fprintf(f,"kwaternion = [%f, %f, %f], w = %f\n",qOrient.x,qOrient.y,qOrient.z,qOrient.w);
  //fprintf(f,"os obrotu = [%f, %f, %f], kat = %f\n",k.x,k.y,k.z,k.w*180.0/PI);

  glRotatef(k.w*180.0/PI,k.x,k.y,k.z);
  glTranslatef(-dlugosc/2,-wysokosc/2,-szerokosc/2);
  glScalef(dlugosc,wysokosc,szerokosc);

  glCallList(Cube);
  GLfloat Surface[] = { 2.0f, 2.0f, 1.0f, 1.0f};
  glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, Surface);
  glRasterPos2f(0.30,1.20);
  glPrint("%d",iID ); 
  glPopMatrix();
}



//**********************
//   Obiekty nieruchome
//**********************
Teren::Teren()
{
  rozmiar_pola = 35;         // d�ugo�� boku kwadratu w [m]           
  float t[][44] = { {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // ostatni element nieu�ywany
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 20, 20,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 80,170,170,200,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  2,  5,  5,  2,  0,  0,  0,  0,  0,  0,  0,120,220,250,200,200,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  3,  6,  9, 12, 12,  4,  0,  0,  0,  0,  0, 40,130,200,250,200,150,  0,  0,  0,  0,  0,  0,  0,100,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  2,  2,  0,  0, 14,  9,  4,  0,  0,  0, 20, 40,120,200,220,150,150,  0,  0,  0,  0, 50, 50,300,300,300,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  1,  1,  0,  1,  1,  1,  2,  2,  0,  0,  8,  4,  0,  0,  0,  0, 20, 40, 90,120,170,  0,  0,  0,  0,  0,  0, 60,300,350,330,300,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0, -1,  2,  0, -3,  0,  2,  1,  2,  0,  0,  0,  0,  0,  0, 10, 20, 30, 40, 50,100,140,  0,  0,  0,  0,  0,  0, 50,300,300,300,150, 50,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0, -1,  2,  0,  0,  0,  0, -1, -1,  2,  0,  0,  0,  0,  0,  0,  0, 10, 10, 40, 70,100,110,  0,  0,  0,  0,  0,  0, 50, 40,300,200, 50, 50,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  1,  0,  0, -1,  0,  0,  0,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 10, 10, 90, 90,  0,  0,  0,  0,  0,  0,  0,100, 40,100, 50, 50,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  1,  0,  0,  0, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,-10,-10, 10, 10,  0,  0,  0,  0,  0,  0,  0,100,100,  0,100, 70, 40,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  1,  0,  0,  0, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,-10, 40, 40,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 70, 70, 30,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 70, 70,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 70, 20, 20,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 70, 70,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 20,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0, -2, -2, -2, -2,  0,  0,  0,  0,  0,  0,  0, -1, -1,  0,  0, 70,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0, -6, -5, -5, -3, -3,  0,  0,  0,  0,  0,  0, -2, -2, -1,  0, 70, 70,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0, -7, -6, -3, -3, -5, -4,  0,  0,  0,  0,  0, -1, -3, -3, -2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0, -8, -8,  0,  0,  0, -4, -2,  0,  0,  0,  0,  0, -2, -3, -3,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0, -8,  0,  0,  0,  0, -2,  0,  0,  0,  0,  0,  0,  0, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},  
  {0,  0,  0,  0,  0,  0,  0, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 10,  0,  0,  0,  0,  0,  0},        
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8, 10,-10,  0,  0,  0,  0,  7,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 10,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8, 10,-20,-10,  0,  0,  0,  7,  7,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 10,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8, 16, 10,-10,  0,  0,  0,  0,  7,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10,  0,  0, 10, 10, 10,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  2, 10, 10, 10, 10,  0,  0,  0,  0,  0,  0,  0,-20,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 10,  0, 10, 10, 10,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  2, 10, 10, 10,  3,  0,  0,  0,  0,  0,  0,-20,-20,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 10, 10, 20, 20, 10,  5,  5,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  2, 10, 10,  5,  0,  0,  0,  0,  0,  0,-40,-40,-20,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 20, 20, 20, 10,  5,  5,  0,  0,  0,  0},
  {0,  0,  0,  0,  0, -3,  0,  0,-10,-10,  0,  2, 10,  5,  0,  0,  1,  0,  0,  0,  0,-40,-40,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 20, 20, 20, 10,  5,  5,  0,  0,  0,  0},
  {0,  0,  0,  0,  0, -3,  0,-13,-10, -6,  0,  0,  5,  0,  0,  1,  3,  0,  0,  0,  0,-40,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 20, 20, 20, 10,  5,  5,  5,  0,  0,  0},
  {0,  0,  0,  0,  0,  0, -3,  0,-18,-16,  0,  0,  0,  0,  0,  0,  2,  3,  5,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 20, 20, 20, 10,  5,  5,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0, -3,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  4,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  3,  5, 10, 20, 20, 20, 10,  5,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0, -3,  0,  0,  0,  0,  0,  0,  0,  0,  2,  3,  5,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  3,  5, 10, 20, 20, 20, 10,  5,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0, -3,  0,  0,  0,  0,  0,  0,  0,  1,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  3,  5, 10, 20, 20, 20, 10,  5,  0, 20, 20,  0,  0},
  {0,  0,  0,  0,  0,  0,  0, -3, -3,  0,  0,  0,  0,  0,  0,  0,  4,  4,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  3,  0, 20, 20, 20, 10,  0,  0, 20, 20,  0,  0},
  {0,  0,  0,  0,  0,  0, -3, -5, -3,  0,  0,  0,  0,  0,  0,  2,  4,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 20, 20, 20, 10,  0,  0, 20, 20,  0,  0},
  {0,  0,  0,  0,  0,  0, -3, -5, -3,  0,  0,  0,  0,  0,  0,  0,  4,  4,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 20, 20, 30, 30, 30, 20, 20, 20, 20,  0},
  {0,  0,  0,  0,  0, -3, -3, -3,  0,  0,  0,  0,  0,  0,  0,  0,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 20, 20, 40, 40, 40, 40, 40, 40, 40, 40,  0},
  {0,  0,  0,  0,  0, -3,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 20, 30, 40, 40, 60, 60, 60, 60, 40, 40,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 20, 20, 30, 40, 40, 60, 60, 60, 60, 40, 40,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0, 20, 20, 30, 40, 40, 60, 60, 60, 60, 40, 40,  0},
  {0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  0,-50,  0,  0,  0,  0,  1, -1,  0,  0,  0,  3,  3,  0,  0,  0,  0,  0,  0,  0,  0,  0, 30, 20, 30, 40, 60, 60, 60, 60, 60, 40, 40,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,-20,-50,  0,  0,  0,  0,  1, -1,  0,  0,  1,  5,  8,  0,  0,  0,  0,  0,  0,  0,  0, 30, 30, 40, 60, 60, 60, 60, 60, 60, 60, 40,  0},
  {0,  0,  0,  0,  0,  0,  2,  2,  2,  1,-20,-20,-30,  0,  0,  0,  0,  1, -1,  0,  0,  2,  5,  9,  0,  0,  0,  0,  0,  0,  0, 20, 30, 40, 60, 60,100,100,100, 60, 60, 60, 40,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,-20,-10,  0,  0,  0,  0,  0,  1, -1,  0,  0,  2,  5,  7,  0,  0,  0,  0,  0,  0, 20, 30, 40, 60,100,100,100,100,100, 60, 60, 40,  0},
  {0,  0,  0,  0,  0,  4,  4,  2,  3,  2,  1, -5,  0,  0,  0,  0,  0,  0,  1, -1,  0,  0,  2,  4,  0,  0,  0,  0,  0,  0,  0, 20, 30, 40, 60,100,100,100,120,100,100, 60, 40,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 20, 30, 40, 60,100,100, 80, 80,100,100, 60, 40,  0},
  {0,  0,  0,  0,  0,  4,  3,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 30, 40, 60,100,100,100, 80,100,100, 60, 40,  0},
  {0,  0,  0,  0,  0,  0,  0,0.5,  1,  1,  0,  0,  0,  0,  0,  0,-30,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 40, 60, 60,100,100,100,100, 60, 60, 40,  0},
  {0,  0,  0,  0,  0,  4,  4,  2,  3,  1,  1,  1,  0,  0,  0,-30,-30,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 20, 60, 60,100,100, 60, 60, 60, 40,  0},          
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  0,  0,  0,  0,-30,-30,-30,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 20, 60, 60, 60, 60, 60, 60, 60, 40,  0},
  {0,  0,  0,  0,  0,  5,  4,  2,  2,  1,  1,  1,  0,  0,  0,-30,-30,-25,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 20, 20, 20, 40, 20, 40, 40, 40, 40,  0},  
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,-25,-22,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 20, 20, 40, 20, 20, 40, 40,  0,  0},
  {0,  0,  0,  0,  0,-20,60,-20,  0,  0,  0,  0,  0,  0,  0, 10,  0,-22,-20,  0,  0, -3,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 20, 20, 20,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0, 70,60,-20,  0,  0,  0,  0,  0,  0, 10, 10,  0,-19,-18,  0, -6, -3,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 20, 20, 20,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0, 65,50,  0,  0,  0,  0,  0,  0,  5, 10,  0,  0,-16,-13, -8, -6,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 20, 20,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,-20,  0,  0,  0,  0,  0,  0,  2,  5,  0,  0,  0,-13,-10, -8,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 20,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,-10,-20,-60,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,-10,-20,-30,-60,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,-10,-40,-90,-60,-60,  0,  0,  0,  0,  0,  0,  0, 10, 10, 10, 10, 10,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,-20,-40,-90,-90,-60,  0,  0,  0,  0,  0,  0, 10, 10, 10, 10, 10, 10,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,-10,-40,-90,-90,-90,-60,  0,  0,  0, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,-10,-50,-90,-90,-90,-60,  0,  0,  0, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,  0,  0,  0,  0,  0,  0,  0,  0, 40, 40,-20,-10,  0,  0,  0,  0,  0},
  {0,-10,-50,-70,-90,-60,-40,  0,  0,  0, 10, 10, 10, 20, 20, 20, 20, 30, 30, 40, 40, 50, 50, 70, 70, 10, 10,  0,  0,  0,  0,  0,  0,  0,  0,  0, 40,-20,-10,  0,  0,  0,  0,  0},
  {0,-10,-20,-40,-40,-40,-40,  0,  0,  0, 10, 10, 10, 20, 20, 20, 20, 30, 30, 40, 40, 50, 50, 70, 70, 10, 10,  0,  0,  0,  0,  0,  0,  0,  0,  0, 40, 40,  0,  0,  0,  0,  0,  0},
  {0,-10,-20,-20,-30,-20,-20,-10,  0,  0, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 40,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,-10,-20,-20,-10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,-10,-10,-10,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}};

  lkolumn = 43;         // o 1 mniej, gdy� kolumna jest r�wnowa�na ci�gowi kwadrat�w 
  lwierszy = sizeof(t)/sizeof(float)/(lkolumn+1)/2-1;                 
  mapa = new float*[lwierszy*2+1];
  for (long i=0;i<lwierszy*2+1;i++) {
    mapa[i] = new float[lkolumn+1];
    for (long j=0;j<lkolumn+1;j++) mapa[i][j] = t[i][j];
  }  	

  for (long i=0;i<lwierszy*2+1;i++)
    for (long j=0;j<lkolumn+1;j++)
      mapa[i][j] /= 3;

  d = new float**[lwierszy];
  for (long i=0;i<lwierszy;i++) {
    d[i] = new float*[lkolumn];
    for (long j=0;j<lkolumn;j++) d[i][j] = new float[4];
  }    
  Norm = new Wektor3**[lwierszy];
  for (long i=0;i<lwierszy;i++) {
    Norm[i] = new Wektor3*[lkolumn];
    for (long j=0;j<lkolumn;j++) Norm[i][j] = new Wektor3[4];
  }    

  fprintf(f,"mapa terenu: lwierszy = %d, lkolumn = %d rozmiar_mapy = %d\n",lwierszy,lkolumn,sizeof(t));

  
  liczba_przedmiotow_max = 1000;
  p = new Przedmiot[liczba_przedmiotow_max];
  p[0].typ = MONETA; p[0].wPol = Wektor3(10,0,10); p[0].do_wziecia = 1; p[0].wartosc = 100;
  p[1].typ = MONETA; p[1].wPol = Wektor3(10,0,-10); p[1].do_wziecia = 1; p[1].wartosc = 100;
  p[2].typ = MONETA; p[2].wPol = Wektor3(-10,0,-20); p[2].do_wziecia = 1; p[2].wartosc = 100;
  p[3].typ = MONETA; p[3].wPol = Wektor3(60,0,-60); p[3].do_wziecia = 1; p[3].wartosc = 500;
  p[4].typ = MONETA; p[4].wPol = Wektor3(-40,0,-20); p[4].do_wziecia = 1; p[4].wartosc = 200;
  p[5].typ = MONETA; p[5].wPol = Wektor3(40,0,50); p[5].do_wziecia = 1; p[5].wartosc = 200;
  p[6].typ = DRZEWO; p[6].podtyp = TOPOLA; p[6].wPol = Wektor3(-40,0,50); p[6].do_wziecia = 0; p[6].wartosc = 10;
  p[7].typ = DRZEWO; p[7].podtyp = TOPOLA; p[7].wPol = Wektor3(40,0,55); p[7].do_wziecia = 0; p[7].wartosc = 14;
  p[8].typ = DRZEWO; p[8].podtyp = TOPOLA; p[8].wPol = Wektor3(45,0,55); p[8].do_wziecia = 0; p[8].wartosc = 10;
  p[9].typ = DRZEWO; p[9].podtyp = TOPOLA; p[9].wPol = Wektor3(45,0,37); p[9].do_wziecia = 0; p[9].wartosc = 8;
  p[10].typ = DRZEWO; p[10].podtyp = SWIERK; p[10].wPol = Wektor3(45,0,5); p[10].do_wziecia = 0; p[10].wartosc = 7;
  p[11].typ = DRZEWO; p[11].podtyp = SWIERK; p[11].wPol = Wektor3(38,0,7); p[11].do_wziecia = 0; p[11].wartosc = 8;
  p[12].typ = DRZEWO; p[12].podtyp = SWIERK; p[12].wPol = Wektor3(8,0,57); p[12].do_wziecia = 0; p[12].wartosc = 8;
  p[13].typ = DRZEWO; p[13].podtyp = SWIERK; p[13].wPol = Wektor3(-2,0,60); p[13].do_wziecia = 0; p[13].wartosc = 8;
  p[14].typ = DRZEWO; p[14].podtyp = SWIERK; p[14].wPol = Wektor3(-8,0,52); p[14].do_wziecia = 0; p[14].wartosc = 12;
  p[15].typ = DRZEWO; p[15].podtyp = TOPOLA; p[15].wPol = Wektor3(107,0,-65); p[15].do_wziecia = 0; p[15].wartosc = 18;
  p[16].typ = DRZEWO; p[16].podtyp = TOPOLA; p[16].wPol = Wektor3(110,0,-75); p[16].do_wziecia = 0; p[16].wartosc = 17;
  p[17].typ = DRZEWO; p[17].podtyp = TOPOLA; p[17].wPol = Wektor3(110,0,-85); p[17].do_wziecia = 0; p[17].wartosc = 19;
  p[19].typ = MONETA; p[19].wPol = Wektor3(80,0,-97); p[19].do_wziecia = 1; p[19].wartosc = 500;
  p[20].typ = MONETA; p[20].wPol = Wektor3(-30,0,-78); p[20].do_wziecia = 1; p[20].wartosc = 200;
  p[21].typ = MONETA; p[21].wPol = Wektor3(40,0,-88); p[21].do_wziecia = 1; p[21].wartosc = 100;
  p[22].typ = MONETA; p[22].wPol = Wektor3(-70,0,-88); p[22].do_wziecia = 1; p[22].wartosc = 100;
  p[23].typ = MONETA; p[23].wPol = Wektor3(-70,0,-68); p[23].do_wziecia = 1; p[23].wartosc = 100;
  p[24].typ = DRZEWO; p[24].podtyp = SWIERK; p[24].wPol = Wektor3(-30,0,-35); p[24].do_wziecia = 0; p[24].wartosc = 8;
  p[25].typ = DRZEWO; p[25].podtyp = SWIERK; p[25].wPol = Wektor3(-25,0,-25); p[25].do_wziecia = 0; p[25].wartosc = 11;
  p[26].typ = MONETA; p[26].wPol = Wektor3(-37,0,-34); p[26].do_wziecia = 1; p[26].wartosc = 1000;
  p[27].typ = DRZEWO; p[27].podtyp = TOPOLA; p[27].wPol = Wektor3(140,0,120); p[27].do_wziecia = 0; p[27].wartosc = 20;
  p[28].typ = DRZEWO; p[28].podtyp = TOPOLA; p[28].wPol = Wektor3(113,0,-97); p[28].do_wziecia = 0; p[28].wartosc = 19;
  p[29].typ = BECZKA; p[29].wPol = Wektor3(-90,0,-88); p[29].do_wziecia = 1; p[29].wartosc = 10;
  p[30].typ = BECZKA; p[30].wPol = Wektor3(-100,0,-68); p[30].do_wziecia = 1; p[30].wartosc = 10;
  p[31].typ = BECZKA; p[31].wPol = Wektor3(120,0,-65); p[31].do_wziecia = 1; p[31].wartosc = 10;
  p[32].typ = BECZKA; p[32].wPol = Wektor3(130,0,-99); p[32].do_wziecia = 1; p[32].wartosc = 50;
  p[33].typ = BECZKA; p[33].wPol = Wektor3(150,0,-78); p[33].do_wziecia = 1; p[33].wartosc = 20;
  p[34].typ = BECZKA; p[34].wPol = Wektor3(-110,0,-130); p[34].do_wziecia = 1; p[34].wartosc = 10;
  p[35].typ = BECZKA; p[35].wPol = Wektor3(123,0,-35); p[35].do_wziecia = 1; p[35].wartosc = 10;
  p[36].typ = BECZKA; p[36].wPol = Wektor3(137,0,-110); p[36].do_wziecia = 1; p[36].wartosc = 40;
  p[37].typ = BECZKA; p[37].wPol = Wektor3(140,0,-118); p[37].do_wziecia = 1; p[37].wartosc = 30;
  p[38].typ = BECZKA; p[38].wPol = Wektor3(12,0,-165); p[38].do_wziecia = 1; p[38].wartosc = 10;
  p[39].typ = BECZKA; p[39].wPol = Wektor3(17,0,-110); p[39].do_wziecia = 1; p[39].wartosc = 20;
  p[40].typ = BECZKA; p[40].wPol = Wektor3(14,0,-118); p[40].do_wziecia = 1; p[40].wartosc = 10;
  p[41].typ = MONETA; p[41].wPol = Wektor3(145,0,-12); p[41].do_wziecia = 1; p[41].wartosc = 200;
  p[42].typ = MONETA; p[42].wPol = Wektor3(147,0,-200); p[42].do_wziecia = 1; p[42].wartosc = 200;
  p[43].typ = MONETA; p[43].wPol = Wektor3(140,0,123); p[43].do_wziecia = 1; p[43].wartosc = 200;
  p[44].typ = MONETA; p[44].wPol = Wektor3(137,0,134); p[44].do_wziecia = 1; p[44].wartosc = 200;
  p[45].typ = DRZEWO; p[45].podtyp = TOPOLA; p[45].wPol = Wektor3(178,0,-194); p[45].do_wziecia = 0; p[45].wartosc = 20;
  p[46].typ = DRZEWO; p[46].podtyp = TOPOLA; p[46].wPol = Wektor3(180,0,-188); p[46].do_wziecia = 0; p[46].wartosc = 19;
  p[47].typ = BECZKA; p[47].wPol = Wektor3(-190,0,158); p[47].do_wziecia = 1; p[47].wartosc = 20;
  p[48].typ = BECZKA; p[48].wPol = Wektor3(-176,0,165); p[48].do_wziecia = 1; p[48].wartosc = 20;
  p[49].typ = BECZKA; p[49].wPol = Wektor3(-140,0,-68); p[49].do_wziecia = 1; p[49].wartosc = 20;
  p[50].typ = BECZKA; p[50].wPol = Wektor3(-136,0,-55); p[50].do_wziecia = 1; p[50].wartosc = 20;
  p[51].typ = MONETA; p[51].wPol = Wektor3(31,0,204); p[51].do_wziecia = 1; p[51].wartosc = 100;
  p[52].typ = MONETA; p[52].wPol = Wektor3(31,0,255); p[52].do_wziecia = 1; p[52].wartosc = 500;
  p[53].typ = MONETA; p[53].wPol = Wektor3(-71,0,40); p[53].do_wziecia = 1; p[53].wartosc = 100;
  p[54].typ = MONETA; p[54].wPol = Wektor3(31,0,75); p[54].do_wziecia = 1; p[54].wartosc = 100;
  p[55].typ = DRZEWO; p[55].podtyp = TOPOLA; p[55].wPol = Wektor3(200,0,-194); p[55].do_wziecia = 0; p[55].wartosc = 20;
  p[56].typ = DRZEWO; p[56].podtyp = TOPOLA; p[56].wPol = Wektor3(320,0,74); p[56].do_wziecia = 0; p[56].wartosc = 20;
  p[57].typ = DRZEWO; p[57].podtyp = TOPOLA; p[57].wPol = Wektor3(320,0,74); p[57].do_wziecia = 0; p[57].wartosc = 20;
  p[58].typ = DRZEWO; p[58].podtyp = TOPOLA; p[58].wPol = Wektor3(328,0,74); p[58].do_wziecia = 0; p[58].wartosc = 24;
  p[59].typ = DRZEWO; p[59].podtyp = SWIERK; p[59].wPol = Wektor3(340,0,64); p[59].do_wziecia = 0; p[59].wartosc = 18;
  p[60].typ = DRZEWO; p[60].podtyp = TOPOLA; p[60].wPol = Wektor3(320,0,84); p[60].do_wziecia = 0; p[60].wartosc = 29;
  p[61].typ = MONETA; p[61].wPol = Wektor3(300,0,94); p[61].do_wziecia = 1; p[61].wartosc = 2000;
  p[62].typ = MONETA; p[62].wPol = Wektor3(290,0,107); p[62].do_wziecia = 1; p[62].wartosc = 2000;
  p[63].typ = DRZEWO; p[63].podtyp = SWIERK; p[63].wPol = Wektor3(310,0,110); p[63].do_wziecia = 0; p[63].wartosc = 23;

  
  liczba_przedmiotow = 64;

  //Przedmiot _p = {{Wektor3(1,2,3),kwaternion(0,0,0,1),MONETA,100,1}}; 
  for (long i = 0;i<liczba_przedmiotow;i++) 
  {
    p[i].czy_odnawialny = 1;
    if ((p[i].typ == MONETA)||(p[i].typ == BECZKA))
      p[i].wPol.y = 1.7 + Wysokosc(p[i].wPol.x,p[i].wPol.z);
  }
}

Teren::~Teren()
{
  for (long i = 0;i< lwierszy*2+1;i++) delete mapa[i];             
  delete mapa;   
  for (long i=0;i<lwierszy;i++)  {
    for (long j=0;j<lkolumn;j++) delete d[i][j];
    delete d[i];
  }
  delete d;  
  for (long i=0;i<lwierszy;i++)  {
    for (long j=0;j<lkolumn;j++) delete Norm[i][j];
    delete Norm[i];
  }
  delete Norm;  

  delete p;         
}

float Teren::Wysokosc(float x,float z)      // okre�lanie wysoko�ci dla punktu o wsp. (x,z) 
{

  float pocz_x = -rozmiar_pola*lkolumn/2,     // wsp�rz�dne lewego g�rnego kra�ca terenu
    pocz_z = -rozmiar_pola*lwierszy/2;        

  long k = (long)((x - pocz_x)/rozmiar_pola), // wyznaczenie wsp�rz�dnych (w,k) kwadratu
    w = (long)((z - pocz_z)/rozmiar_pola);
  if ((k < 0)||(k >= lkolumn)||(w < 0)||(w >= lwierszy)) return 0;  // je�li poza map�

  // wyznaczam punkt B - �rodek kwadratu oraz tr�jk�t, w kt�rym znajduje si� punkt
  // (rysunek w Teren::PoczatekGrafiki())
  Wektor3 B = Wektor3(pocz_x + (k+0.5)*rozmiar_pola, mapa[w*2+1][k], pocz_z + (w+0.5)*rozmiar_pola); 
  enum tr{ABC=0,ADB=1,BDE=2,CBE=3};       // tr�jk�t w kt�rym znajduje si� punkt 
  int trojkat=0; 
  if ((B.x > x)&&(fabs(B.z - z) < fabs(B.x - x))) trojkat = ADB;
  else if ((B.x < x)&&(fabs(B.z - z) < fabs(B.x - x))) trojkat = CBE;
  else if ((B.z > z)&&(fabs(B.z - z) > fabs(B.x - x))) trojkat = ABC;
  else trojkat = BDE;

  // wyznaczam normaln� do p�aszczyzny a nast�pnie wsp�czynnik d z r�wnania p�aszczyzny
  float dd = d[w][k][trojkat];
  Wektor3 N = Norm[w][k][trojkat];
  float y;
  if (N.y > 0) y = (-dd - N.x*x - N.z*z)/N.y;
  else y = 0;

  //fprintf(f,"kwadr = (w=%d,k=%d), trojk = %d,  (x,z)=(%f,%f) y = %f\n",w,k,trojkat,x,z,y);
  //fprintf(f,"kwadr = (w=%d,k=%d), trojk = %d, N=(%f, %f, %f)\n",w,k,trojkat,N.x,N.y,N.z);

  return y;    
}

void Teren::PoczatekGrafiki()
{
  // tworze list� wy�wietlania rysuj�c poszczeg�lne pola mapy za pomoc� tr�jk�t�w 
  // (po 4 tr�jk�ty na ka�de pole):
  enum tr{ABC=0,ADB=1,BDE=2,CBE=3};       
  float pocz_x = -rozmiar_pola*lkolumn/2,     // wsp�rz�dne lewego g�rnego kra�ca terenu
    pocz_z = -rozmiar_pola*lwierszy/2;        
  Wektor3 A,B,C,D,E,N;      
  glNewList(PowierzchniaTerenu,GL_COMPILE);
  glBegin(GL_TRIANGLES);
  for (long w=0;w<lwierszy;w++) 
    for (long k=0;k<lkolumn;k++) 
    {
      A = Wektor3(pocz_x + k*rozmiar_pola, mapa[w*2][k], pocz_z + w*rozmiar_pola);
      B = Wektor3(pocz_x + (k+0.5)*rozmiar_pola, mapa[w*2+1][k], pocz_z + (w+0.5)*rozmiar_pola);            
      C = Wektor3(pocz_x + (k+1)*rozmiar_pola, mapa[w*2][k+1], pocz_z + w*rozmiar_pola); 
      D = Wektor3(pocz_x + k*rozmiar_pola, mapa[(w+1)*2][k], pocz_z + (w+1)*rozmiar_pola);       
      E = Wektor3(pocz_x + (k+1)*rozmiar_pola, mapa[(w+1)*2][k+1], pocz_z + (w+1)*rozmiar_pola); 
      // tworz� tr�jk�t ABC w g�rnej cz�ci kwadratu: 
      //  A o_________o C
      //    |.       .|
      //    |  .   .  | 
      //    |    o B  | 
      //    |  .   .  |
      //    |._______.|
      //  D o         o E

      Wektor3 AB = B-A;
      Wektor3 BC = C-B;
      N = (AB*BC).znorm();          
      glNormal3f( N.x, N.y, N.z);
      glVertex3f( A.x, A.y, A.z);
      glVertex3f( B.x, B.y, B.z);
      glVertex3f( C.x, C.y, C.z);
      d[w][k][ABC] = -(B^N);          // dodatkowo wyznaczam wyraz wolny z r�wnania plaszyzny tr�jk�ta
      Norm[w][k][ABC] = N;          // dodatkowo zapisuj� normaln� do p�aszczyzny tr�jk�ta
      // tr�jk�t ADB:
      Wektor3 AD = D-A;
      N = (AD*AB).znorm();          
      glNormal3f( N.x, N.y, N.z);
      glVertex3f( A.x, A.y, A.z);
      glVertex3f( D.x, D.y, D.z);
      glVertex3f( B.x, B.y, B.z);
      d[w][k][ADB] = -(B^N);       
      Norm[w][k][ADB] = N;
      // tr�jk�t BDE:
      Wektor3 BD = D-B;
      Wektor3 DE = E-D;
      N = (BD*DE).znorm();          
      glNormal3f( N.x, N.y, N.z);
      glVertex3f( B.x, B.y, B.z);
      glVertex3f( D.x, D.y, D.z);     
      glVertex3f( E.x, E.y, E.z);  
      d[w][k][BDE] = -(B^N);        
      Norm[w][k][BDE] = N;  
      // tr�jk�t CBE:
      Wektor3 CB = B-C;
      Wektor3 BE = E-B;
      N = (CB*BE).znorm();          
      glNormal3f( N.x, N.y, N.z);
      glVertex3f( C.x, C.y, C.z);
      glVertex3f( B.x, B.y, B.z);
      glVertex3f( E.x, E.y, E.z);      
      d[w][k][CBE] = -(B^N);        
      Norm[w][k][CBE] = N;
    }		
    glEnd();
    glEndList(); 

    /*glNewList(PowierzchniaTerenu,GL_COMPILE);
    glBegin(GL_POLYGON);
    glNormal3f( 0.0, 1.0, 0.0);
    glVertex3f( -100, 0, -300.0);
    glVertex3f( -100, 0, 100.0);
    glVertex3f( 100, 0, 100.0);
    glVertex3f( 100, 0, -300.0);
    glEnd();
    glEndList();*/

}



void Teren::Rysuj()
{
  // rysowanie powierzchni terenu:
  glCallList(PowierzchniaTerenu);

  // rysowanie r�nych przedmiot�w (monet, budynk�w, drzew)
  GLUquadricObj *Qcyl = gluNewQuadric();
  GLUquadricObj *Qdisk = gluNewQuadric();
  GLUquadricObj *Qsph = gluNewQuadric();
  for (long i=0;i<liczba_przedmiotow;i++)
  {
    glPushMatrix();

    glTranslatef(p[i].wPol.x,p[i].wPol.y,p[i].wPol.z);
    //glRotatef(k.w*180.0/PI,k.x,k.y,k.z);
    //glScalef(dlugosc,wysokosc,szerokosc);
    switch (p[i].typ)
    {
    case MONETA:
      //gluCylinder(Qcyl,promien1,promien2,wysokosc,10,20);
      if (p[i].do_wziecia)
      {
        GLfloat Surface[] = { 2.0f, 2.0f, 1.0f, 1.0f};
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, Surface);
        glRotatef(90,1,0,0);
        p[i].srednica = powf(p[i].wartosc/100,0.4); float grubosc = 0.2*p[i].srednica;
        p[i].wPol.y = grubosc + Wysokosc(p[i].wPol.x,p[i].wPol.z);
        gluDisk(Qdisk,0,p[i].srednica,10,10);
        gluCylinder(Qcyl,p[i].srednica,p[i].srednica,grubosc,10,20); 
        glRasterPos2f(0.30,1.20);
        glPrint("%d",p[i].wartosc); 
      }
      break;
    case BECZKA:
      //gluCylinder(Qcyl,promien1,promien2,wysokosc,10,20);
      if (p[i].do_wziecia)
      {
        GLfloat Surface[] = { 0.50f, 0.45f, 0.0f, 1.0f};
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, Surface);
        glRotatef(90,1,0,0);
        p[i].srednica = powf((float)p[i].wartosc/50,0.4); float grubosc = 2*p[i].srednica;
        p[i].wPol.y = grubosc + Wysokosc(p[i].wPol.x,p[i].wPol.z);
        gluDisk(Qdisk,0,p[i].srednica,10,10);
        gluCylinder(Qcyl,p[i].srednica,p[i].srednica,grubosc,10,20); 
        glRasterPos2f(0.30,1.20);
        glPrint("%d",p[i].wartosc); 
      }
      break;
    case DRZEWO:
      switch (p[i].podtyp)
      {
      case TOPOLA:  
        {     
          GLfloat Surface[] = { 0.5f, 0.5f, 0.0f, 1.0f};
          glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, Surface);
          glPushMatrix();
          glRotatef(90,1,0,0);
          p[i].srednica = 0.8; float wysokosc = p[i].wartosc;
          p[i].wPol.y = wysokosc + Wysokosc(p[i].wPol.x,p[i].wPol.z);
          gluCylinder(Qcyl,p[i].srednica/2,p[i].srednica,wysokosc,10,20); 
          glPopMatrix();
          GLfloat Surface2[] = { 0.0f, 0.9f, 0.0f, 1.0f};
          glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, Surface2);
          //glTranslatef(0,wysokosc,0);
          glScalef(1,2,1);
          gluSphere(Qsph,3,20,20);
          break;
        }	
      case SWIERK:	
        {
          GLfloat Surface[] = { 0.65f, 0.3f, 0.0f, 1.0f};
          glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, Surface);
          glPushMatrix();
          glRotatef(90,1,0,0);
          float wysokosc = p[i].wartosc;
          p[i].srednica = wysokosc/10;
          float promien = p[i].srednica/2;
          p[i].wPol.y = wysokosc + Wysokosc(p[i].wPol.x,p[i].wPol.z);
          gluCylinder(Qcyl,promien,promien*2,wysokosc,10,20); 

          GLfloat Surface2[] = { 0.0f, 0.70f, 0.2f, 1.0f};
          glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, Surface2);
          glTranslatef(0,0,wysokosc*2/4);
          gluCylinder(Qcyl,promien*2,promien*8,wysokosc/4,10,20); 
          glTranslatef(0,0,-wysokosc*1/4);
          gluCylinder(Qcyl,promien*2,promien*6,wysokosc/4,10,20); 
          glTranslatef(0,0,-wysokosc*1/3);
          gluCylinder(Qcyl,0,promien*4,wysokosc/3,10,20); 
          glPopMatrix(); 
        }
      }	
    }

    glPopMatrix();
  }
  gluDeleteQuadric(Qcyl);gluDeleteQuadric(Qdisk);gluDeleteQuadric(Qsph);
  //glCallList(Floor);                   
}


