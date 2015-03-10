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
extern int iLiczbaInnychOb;
extern ObiektRuchomy *InneObiekty[2000]; 


ObiektRuchomy::ObiektRuchomy()             // konstruktor                   
{

	//iID = (unsigned int)(clock() % 1000);  // identyfikator obiektu
	iID = (unsigned int)(rand() % 1000);  // identyfikator obiektu
    fprintf(f,"MojObiekt->iID = %d\n",iID);


	F = Fb = alfa  = 0;	// si�y dzia�aj�ce na obiekt 
	ham = 0;			// stopie� hamowania
	m = 1.0;			// masa obiektu
	Fy = m*9.81;        // si�a nacisku na podstaw� obiektu (na ko�a pojazdu)
	dlugosc = 7.1;
	szerokosc = 2.3;
	wysokosc = 1.3;
	przeswit = 0.0;     // wysoko�� na kt�rej znajduje si� podstawa obiektu
	dl_przod = 1.3;     // odleg�o�� od przedniej osi do przedniego zderzaka 
	dl_tyl = 0.2;       // odleg�o�� od tylniej osi do tylniego zderzaka
	
  wPol.y = przeswit+wysokosc/2 + 20;
  wPol.x = -50;
  //wV_kat = Wektor3(0,1,0)*40;  // pocz�tkowa pr�dko�� k�towa (w celach testowych)

	// obr�t obiektu o k�t 30 stopni wzgl�dem osi y:
	kwaternion qObr = AsixToQuat(Wektor3(0,1,0),0.1*PI/180.0);
	qOrient = qObr*qOrient;
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
	return stan;
}

void ObiektRuchomy::Symulacja(float dt)          // obliczenie nowego stanu na podstawie dotychczasowego,
{                                                // dzia�aj�cych si� i czasu, jaki up�yn�� od ostatniej symulacji

  if (dt == 0) return;

  float tarcie = 2.5;            // wsp�czynnik tarcia obiektu o pod�o�e 
  float tarcie_obr = tarcie;     // tarcie obrotowe (w szczeg�lnych przypadkach mo�e by� inne ni� liniowe)
  float tarcie_toczne = 0.05;    // wsp�czynnik tarcia tocznego 
  float g = 9.81;                // przyspieszenie grawitacyjne

  // obliczam wektor skierowany do przodu obiektu, by przesun�� obiekt w kierunku tego wektora 
  // proporcjonalnie do si�y F:
  Wektor3 w_przod = Wektor3(1,0,0);		// na razie o� obiektu pokrywa si� z osi� x globalnego uk�adu wsp�rz�dnych (lokalna o� x)
  Wektor3 w_gora = Wektor3(0,1,0);		// wektor skierowany pionowo w g�r� od podstawy obiektu (lokalna o� y)
  Wektor3 w_prawo = Wektor3(0,0,1);       // wektor skierowany w prawo (lokalna o� z)

  // obracam uk�ad wsp�rz�dnych lokalnych wed�ug kwaterniona orientacji:
  w_przod = qOrient.obroc_wektor(w_przod);
  w_gora = qOrient.obroc_wektor(w_gora);
  w_prawo = qOrient.obroc_wektor(w_prawo);

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
    Wektor3 wobrot = -norm.znorm()*w_gora;  
    wV_kat_wpoziomie = wobrot/dt; 
  }    

  // jesli wiecej niz 2 kola sa na ziemi, to przyspieszenie grawitacyjne jest rownowazone przez opor gruntu:
  if ((P.y <= Pt.y + wysokosc/2+przeswit)+(Q.y <= Qt.y  + wysokosc/2+przeswit)+
    (R.y <= Rt.y  + wysokosc/2+przeswit)+(S.y <= St.y  + wysokosc/2+przeswit) > 2)
  {	
    g = 0; 
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
  wA = (w_przod*F + w_prawo*Fb)/m*(Fy>0) - wV_wprzod.znorm()*(Fh/m + tarcie_toczne*Fy/m)*(V_wprzod>0.01)
    - wV_wprawo.znorm()*tarcie*Fy/m*(V_wprawo>0.01)- Wektor3(0,1,0)*g;

  // obliczenie nowej orientacji:
  Wektor3 w_obrot = wV_kat*dt;// + wA_kat*dt*dt/2;    
  kwaternion q_obrot = AsixToQuat(w_obrot.znorm(),w_obrot.dlugosc());
  //fprintf(f,"w_obrot = (x=%f, y=%f, z=%f) \n",w_obrot.x, w_obrot.y, w_obrot.z );
  //fprintf(f,"q_obrot = (w=%f, x=%f, y=%f, z=%f) \n",q_obrot.w, q_obrot.x, q_obrot.y, q_obrot.z );
  qOrient = q_obrot*qOrient; 	
  fprintf(f,"Pol = (%f, %f, %f) V = (%f, %f, %f) A = (%f, %f, %f) V_kat = (%f, %f, %f)\n",
    wPol.x,wPol.y,wPol.z,wV.x,wV.y,wV.z,wA.x,wA.y,wA.z,wV_kat.x,wV_kat.y,wV_kat.z);

}

void ObiektRuchomy::Rysuj()
{
  glPushMatrix();

  glTranslatef(wPol.x,wPol.y+przeswit,wPol.z);

  Wektor3 k = qOrient.AsixAngle();

  glRotatef(k.dlugosc()*180.0/PI,k.x,k.y,k.z);
  glTranslatef(-dlugosc/2,-wysokosc/2,-szerokosc/2);
  glScalef(dlugosc,wysokosc,szerokosc);

  glCallList(Auto);
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
  rozmiar_pola = 24;         // d�ugo�� boku kwadratu w [m] 
  float t[][29] = { {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // ostatni element nieu�ywany
					          {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
					          {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
					          {0,  0,  0,  0, 10,  0,  0,  0,  0,  0,  0,  2,  5,  5,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0, 10, 10,  0,  0,  0,  0,  1,  3,  6,  9, 12, 12,  4,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
					          {0,  0,  0,  0, 10, 10,  0,  0,  0,  0,  0,  0,  0,  0, 14,  9,  4,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0, 10,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,  4,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
					          {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
					          {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
					          {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
					            {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0,  0,  0, -2, -2, -2, -2,  0,  0,  0,  0,  0,  0,  0, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0},
                    {0,  0,  0,  0,  0,  0,  0,  0, -6, -5, -5, -3, -3,  0,  0,  0,  0,  0,  0, -2, -2, -1,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0,  0, -7, -6, -3, -3, -5, -4,  0,  0,  0,  0,  0, -1, -3, -3, -2,  0,  0,  0,  0,  0,  0,  0},
                    {0,  0,  0,  0,  0,  0,  0, -8, -8,  0,  0,  0, -4, -2,  0,  0,  0,  0,  0, -2, -3, -3,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0,  0, -8,  0,  0,  0,  0, -2,  0,  0,  0,  0,  0,  0,  0, -1,  0,  0,  0,  0,  0,  0,  0,  0},  
                    {0,  0,  0,  0,  0,  0,  0, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},        
                    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8, 10,-10,  0,  0,  0,  0,  7,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8, 10,-20,-10,  0,  0,  0,  7,  7,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8, 16, 10,-10,  0,  0,  0,  0,  7,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0,  0,  0,  0,  2, 10, 10, 10, 10,  0,  0,  0,  0,  0,  0,  0,-20,  0,  0,  0,  0,  0,  0,  0},
                    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  2, 10, 10, 10,  3,  0,  0,  5,  0,  0,  0,-20,-20,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  2, 10, 10,  5,  0,  0,  5, 10,  0, 45,-40,-40,-20,  0,  0,  0,  0,  0,  0},
                    {0,  0,  0,  0,  0, -3,  0,  0,-10,-10,  0,  2, 10,  5,  0,  0,  1, 10, 15, 35, 45,-40,-40,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0, -3,  0,-13,-10, -6,  0,  0,  5,  0,  0,  1,  3, 15, 25, 35, 45,-40,  0,  0,  0,  0,  0,  0,  0},
                    {0,  0,  0,  0,  0,  0, -3,  0,-18,-16,  0,  0,  0,  0,  0,  0,  2,  3, 15, 25, 35, 25,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0, -3,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  5, 15, 25, 10,  0,  0,  0,  0,  0,  0,  0,  0},
                    {0,  0,  0,  0,  0,  0,  0, -3,  0,  0,  0,  0,  0,  0,  0,  0,  2,  3,  5, 15, 10, 10,  0,  0, 10,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0,  0, -3,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  5,  0, 10, 15,  0, 10, 10,  0,  0,  0,  0},
                    {0,  0,  0,  0,  0,  0,  0, -3, -3,  0,  0,  0,  0,  0,  0,  0,  4,  4,  3,  0,  0, 15, 10, 10, 10,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0, -3, -5, -3,  0,  0,  0,  0,  0,  0,  2,  4,  2,  0,  0,  0, 10, 10, 10,  0,  0,  0,  0,  0},
                    {0,  0,  0,  0,  0,  0, -3, -5, -3,  0,  0,  0,  0,  0,  0,  0,  4,  4,  0,  0,  0,  0, 10,  5,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0, -3, -3, -3,  0,  0,  0,  0,  0,  0,  0,  0,  2,  0,  0,  0,  0,  0,  5,  0,  0,  0,  0,  0,  0},
                    {0,  0,  0,  0,  0, -3,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  0,-50,  0,  0,  0,  0,  1, -1,  0,  0,  0,  3,  3,  0,  0,  0,  0,  0,  0},
                    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,-20,-50,  0,  0,  0,  0,  1, -1,  0,  0,  1,  5,  8,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0,  2,  2,  2,  1,-20,-20,-30,  0,  0,  0,  0,  1, -1,  0,  0,  2,  5,  9,  0,  0,  0,  0,  0},
                    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,-20,-10,  0,  0,  0,  0,  0,  1, -1,  0,  0,  2,  5,  7,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  4,  4,  2,  3,  2,  1, -5,  0,  0,  0,  0,  0,  0,  1, -1,  0,  0,  2,  4,  0,  0,  0,  0,  0},
                    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1, -1,  0,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  4,  3,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                    {0,  0,  0,  0,  0,  0,  0,0.5,  1,  1,  0,  0,  0,  0,  0,  0,-30,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  4,  4,  2,  3,  1,  1,  1,  0,  0,  0,-30,-30,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},          
                    {0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  0,  0,  0,  0,-30,-30,-30,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
					            {0,  0,  0,  0,  0,  5,  4,  2,  2,  1,  1,  1,  0,  0,  0,-30,-30,-25,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},  
					          {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,-25,-22,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,-20,60,-20,  0,  0,  0,  0,  0,  0,  0, 10,  0,-22,-20,  0,  0, -3,  0,  0,  0,  0,  0,  0,  0},
					          {0,  0,  0,  0,  0,  0, 70,60,-20,  0,  0,  0,  0,  0,  0, 10, 10,  0,-19,-18,  0, -6, -3,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0, 65,50,  0,  0,  0,  0,  0,  0,  5, 10,  0,  0,-16,-13, -8, -6,  0,  0,  0,  0,  0,  0,  0},
					          {0,  0,  0,  0,  0,  0,  0,-20,  0,  0,  0,  0,  0,  0,  2,  5,  0,  0,  0,-13,-10, -8,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
					            {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
					            {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                    {0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
					            {0,  0,  0,  0,  0,  0,  2,  3,  3,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                    {0,  0,  0,  0,  0,  0,  5,  5,  7,  5,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}};
                    
   lkolumn = 28;         // o 1 mniej, gdy� kolumna jest r�wnowa�na ci�gowi kwadrat�w 
   lwierszy = (sizeof(t)/sizeof(float)/(lkolumn+1) + 1)/2-1;                 
   mapa = new float*[lwierszy*2+1];
   for (long i=0;i<lwierszy*2+1;i++) {
     mapa[i] = new float[lkolumn+1];
     for (long j=0;j<lkolumn+1;j++) mapa[i][j] = t[i][j];
   }  	
   
   for (long i=0;i<lwierszy*2+1;i++)
     for (long j=0;j<lkolumn+1;j++)
       mapa[i][j] /= 1.5;
   
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

       
   fprintf(f,"mapa terenu: lwierszy = %d, lkolumn = %d, liczba linii t = %d\n",lwierszy,lkolumn, sizeof(t)/sizeof(float)/(lkolumn+1));
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

         
}

float Teren::Wysokosc(float x,float z)      // okre�lanie wysoko�ci dla punktu o wsp. (x,z) 
{
  
  float pocz_x = -rozmiar_pola*lkolumn/2,     // wsp�rz�dne lewego g�rnego kra�ca terenu
        pocz_z = -rozmiar_pola*lwierszy/2;        
  
  long k = (long)((x - pocz_x)/rozmiar_pola), // wyznaczenie wsp�rz�dnych (w,k) kwadratu
       w = (long)((z - pocz_z)/rozmiar_pola);

  //fprintf(f,"kwadr = (w=%d,k=%d)\n",w,k); 
  //fclose(f);
  //f=fopen("plik.txt","a");

  if ((w < 0)||(w >= lwierszy)||(k < 0)||(k >= lkolumn)) return -1e10;  // je�li poza map�
  
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
  glBegin(GL_QUADS);
    for (long ix=-1;ix<2;ix++)
      for (long iz=-1;iz<2;iz++)
      if ((ix != 0)||(iz != 0))
      {
        glNormal3f(0,1,0);
        glVertex3f(ix*rozmiar_pola*lkolumn + pocz_x , 0, iz*rozmiar_pola*lwierszy + pocz_z);
        glVertex3f((ix+1)*rozmiar_pola*lkolumn + pocz_x , 0, iz*rozmiar_pola*lwierszy + pocz_z);
        glVertex3f((ix+1)*rozmiar_pola*lkolumn + pocz_x , 0, (iz+1)*rozmiar_pola*lwierszy + pocz_z);
        glVertex3f(ix*rozmiar_pola*lkolumn + pocz_x , 0, (iz+1)*rozmiar_pola*lwierszy + pocz_z);
      }
  glEnd();

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
  glCallList(PowierzchniaTerenu);                 
}

   
