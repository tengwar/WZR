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
extern bool czy_rysowac_ID;


ObiektRuchomy::ObiektRuchomy()             // konstruktor                   
{
	iID = (unsigned int)(rand() % 1000);  // identyfikator obiektu
    fprintf(f,"MojObiekt->iID = %d\n",iID);

	F = Fb = alfa  = 0;	// si�y dzia�aj�ce na obiekt 
	ham = 0;			// stopie� hamowania
	m = 5.0;			// masa obiektu [kg]
	Fy = m*9.81;        // si�a nacisku na podstaw� obiektu (na ko�a pojazdu)
	dlugosc = 10.0;
	szerokosc = 4.0;
	wysokosc = 1.6;
	przeswit = 0.0;     // wysoko�� na kt�rej znajduje si� podstawa obiektu
	dl_przod = 1.0;     // odleg�o�� od przedniej osi do przedniego zderzaka 
	dl_tyl = 0.2;       // odleg�o�� od tylniej osi do tylniego zderzaka
	
  wPol.y = przeswit+wysokosc/2 + 25; // wysoko�� �rodka ci�ko�ci w osi pionowej pojazdu
  wPol.x = 0;
  wPol.z = 0;

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

  float tarcie = 3.0;            // wsp�czynnik tarcia obiektu o pod�o�e 
	float tarcie_obr = tarcie;     // tarcie obrotowe (w szczeg�lnych przypadkach mo�e by� inne ni� liniowe)
  float tarcie_toczne = 0.08;    // wsp�czynnik tarcia tocznego
  float sprezystosc = 0.5;       // wsp�czynnik spr�ysto�ci (0-brak spr�ysto�ci, 1-doskona�a spr�ysto��) 
  float g = 9.81;                // przyspieszenie grawitacyjne
       
  // obracam uk�ad wsp�rz�dnych lokalnych wed�ug kwaterniona orientacji:
	Wektor3 w_przod = qOrient.obroc_wektor(Wektor3(1,0,0)); // na razie o� obiektu pokrywa si� z osi� x globalnego uk�adu wsp�rz�dnych (lokalna o� x)
	Wektor3 w_gora = qOrient.obroc_wektor(Wektor3(0,1,0));  // wektor skierowany pionowo w g�r� od podstawy obiektu (lokalna o� y)
	Wektor3 w_prawo = qOrient.obroc_wektor(Wektor3(0,0,1)); // wektor skierowany w prawo (lokalna o� z)
	

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
    wAg = wAg + w_gora*(w_gora^wAg)*-1; //przyspieszenie wynikaj�ce z si�y oporu gruntu
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
  Wektor3 dwPol = wV*dt + wA*dt*dt/2; // czynnik bardzo ma�y - im wi�ksza cz�stotliwo�� symulacji, tym mniejsze znaczenie 
  wPol = wPol + dwPol;  
  
  // korekta po�o�enia w przypadku terenu cyklicznego:
  if (wPol.x < -teren.rozmiar_pola*teren.lkolumn/2) wPol.x += teren.rozmiar_pola*teren.lkolumn;
  else if (wPol.x > teren.rozmiar_pola*(teren.lkolumn-teren.lkolumn/2)) wPol.x -= teren.rozmiar_pola*teren.lkolumn;
  if (wPol.z < -teren.rozmiar_pola*teren.lwierszy/2) wPol.z += teren.rozmiar_pola*teren.lwierszy;
  else if (wPol.z > teren.rozmiar_pola*(teren.lwierszy-teren.lwierszy/2)) wPol.z -= teren.rozmiar_pola*teren.lwierszy;

  // Sprawdzenie czy obiekt mo�e si� przemie�ci� w zadane miejsce: Je�li nie, to 
	// przemieszczam obiekt do miejsca zetkni�cia, wyznaczam nowe wektory pr�dko�ci
	// i pr�dko�ci k�towej, a nast�pne obliczam nowe po�o�enie na podstawie nowych
	// pr�dko�ci i pozosta�ego czasu. Wszystko powtarzam w p�tli (pojazd znowu mo�e 
	// wjecha� na przeszkod�). Problem z zaokr�glonymi przeszkodami - konieczne 
	// wyznaczenie minimalnego kroku.
      
      
  Wektor3 wV_pop = wV;  

  // sk�adam pr�dko�ci w r�nych kierunkach oraz efekt przyspieszenia w jeden wektor:    (problem z przyspieszeniem od si�y tarcia -> to przyspieszenie 
	//      mo�e dzia�a� kr�cej ni� dt -> trzeba to jako� uwzgl�dni�, inaczej pojazd b�dzie w�ykowa�)
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
	 
	// sk�adam przyspieszenia liniowe od si� nap�dzaj�cych i od si� oporu: 
	wA = (w_przod*F + w_prawo*Fb)/m*(Fy>0)  // od si� nap�dzaj�cych
         - wV_wprzod.znorm()*(Fh/m + tarcie_toczne*Fy/m)*(V_wprzod>0.01) // od hamowania i tarcia tocznego (w kierunku ruchu)
         - wV_wprawo.znorm()*tarcie*Fy/m*(V_wprawo>0.01)    // od tarcia w kierunku prost. do kier. ruchu
         + wAg;           // od grawitacji
	

	// obliczenie nowej orientacji:
	Wektor3 w_obrot = wV_kat*dt + wA_kat*dt*dt/2;    
	kwaternion q_obrot = AsixToQuat(w_obrot.znorm(),w_obrot.dlugosc());
	qOrient = q_obrot*qOrient; 		
}

void ObiektRuchomy::Rysuj()
{
	glPushMatrix();
    
	
	glTranslatef(wPol.x,wPol.y+przeswit,wPol.z);
	
	Wektor3 k = qOrient.AsixAngle();     // reprezentacja k�towo-osiowa kwaterniona
	
  Wektor3 k_znorm = k.znorm();

  glRotatef(k.dlugosc()*180.0/PI,k_znorm.x,k_znorm.y,k_znorm.z);
    glTranslatef(-dlugosc/2,-wysokosc/2,-szerokosc/2);
    glScalef(dlugosc,wysokosc,szerokosc);
    	
  glCallList(Auto);
	GLfloat Surface[] = { 2.0f, 2.0f, 1.0f, 1.0f};
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, Surface);
  if (czy_rysowac_ID){
	  glRasterPos2f(0.30,1.20);
    glPrint("%d",iID ); 
  }
	glPopMatrix();
}




//**********************
//   Obiekty nieruchome
//**********************
Teren::Teren()
{
  rozmiar_pola = 40;         // d�ugo�� boku kwadratu w [m]           
  float t[][41] = { {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // ostatni element nieu�ywany
					          {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  5,  5,  0,  0,  0,  0,  0,  0,  0,  0,  8,  8,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
					          {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  5, 10,  5,  0,  0,  0,  0,  0,  0,  0,  8,  8,  8,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  5, 10, 10,  5,  0,  0,  8,  0,  0,  0,  8,  8,  8,  8,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
					          {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  2,  5,  9, 20, 10,  7,  0,  8,  8,  0,  0,  8,  8,  8,  8,  8,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  1,  0,  0,  0,  0,  0,  1,  3,  6,  9, 12, 12,  4,  7,  8,  8,  8,  4,  8,  8, 20, 20,  8,  8,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
					          {0,  0,  0,  0,  0,  0,  0,  0,  0,  8,  4,  4,  6,  8, 14,  9,  8,  8,  8,  8,  4,  8,  8, 20, 20, 20,  8,  8,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0,  0,  0,  8,  8,  8,  5,  8,  8,  8,  8,  8,  8,  8,  8,  8, 28, 20, 20, 40, 20,  8,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
					          {0,  0,  0,  1,  0,  0,  0,  0,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8, 28, 28, 20, 40, 40, 20,  8,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0,  0,  0,  8,  8,  8,  8,  8,  8,  8,  8,  0,  0,  0,  8,  8, 28, 20, 20, 40, 30, 20,  8,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
					          {0,  0,  0,  0,  0,  0,  0,  0,  0,  8,  8,  8,  8,  8,  8,  8,  9,  0,  0,  0,  8,  8,  8, 20, 20, 50, 20,  8,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0,  0,  0,  0,  8,  8,  8,  8,  8,  0,  0,  0,  0,  0,  0,  0,  8,  8, 20, 20, 20,  8,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
					          {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,  8,  8, 20,  8,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
					            {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,  8,  8,  8,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0,  0,  0, -2, -2, -2, -2,  0,  0,  0,  0,  0,  0,  0, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                    {0,  0,  0,  0,  0,  0,  0,  0, -6, -5, -5, -3, -3,  0,  0,  0,  0,  0,  0, -2, -2, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0,  0, -7, -6, -3, -3, -5, -4,  0,  0,  0,  0,  0, -1, -3, -3, -2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                    {0,  0,  0,  0,  0,  0,  0, -8, -8,  0,  0,  0, -4, -2,  0,  0,  0,  0,  0, -2, -3, -3,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0,  0, -8,  0,  0,  0,  0, -2,  0,  0,  0,  0,  0,  0,  0, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},  
                    {0,  0,  0,  0,  0,  0,  0, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},        
                    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8, 10,-10,  0,  0,  0,  0,  7,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8, 10,-20,-10,  0,  0,  0,  7,  7,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8, 16, 10,-10,  0,  0,  0,  0,  7,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0,  0,  0,  0,  2, 10, 10, 10, 10,  0,  0,  0,  0,  0,  0,  0,-20,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  2, 10, 10, 10,  3,  0,  0,  5,  0,  0,  0,-20,-20,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  2, 10, 10,  5,  0,  0,  5, 10,  0, 45,-40,-40,-20,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                    {0,  0,  0,  0,  0, -3,  0,  0,-10,-10,  0,  2, 10,  5,  0,  0,  1, 10, 15, 35, 45,-40,-40,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0, -3,  0,-13,-10, -6,  0,  0,  5,  0,  0,  1,  3, 15, 25, 35, 45,-40,  0, 10, 10, 10, 10,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                    {0,  0,  0,  0,  0,  0, -3,  0,-18,-16,  0,  0,  0,  0,  0,  0,  2,  3, 15, 25, 35, 25,  0,  0, 10, 10, 10, 10,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0, -3,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  5, 15, 25, 10,  0,  0,  0, 10, 10, 10, 10,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                    {0,  0,  0,  0,  0,  0,  0, -3,  0,  0,  0,  0,  0,  0,  0,  0,  2,  3,  5, 15, 10, 10,  0,  0, 10, 10, 10, 10,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0,  0, -3,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  5,  0, 10, 15,  0, 10, 10, 10, 10,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                    {0,  0,  0,  0,  0,  0,  0, -3, -3,  0,  0,  0,  0,  0,  0,  0,  4,  4,  3,  0,  0, 15, 10, 10, 10, 10, 10,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0, -3, -5, -3,  0,  0,  0,  0,  0,  0,  2,  4,  2,  0,  0,  0, 10, 10, 10, 10, 10, 10,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                    {0,  0,  0,  0,  0,  0, -3, -5, -3,  0,  0,  0,  0,  0,  0,  0,  4,  4,  0,  0,  0,  0, 10, 10, 10, 20, 10,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0, -3, -3, -3,  0,  0,  0,  0,  0,  0,  0,  0,  2,  0,  0,  0,  0,  0, 10, 10, 20, 20, 10,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                    {0,  0,  0,  0,  0, -3,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 10, 20, 10,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 10, 10, 10,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  0, 10,  0,  0,  0,  0,  0,  0, 10, 10,  0,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  0,-50,  0,  0,  0,  0,  1, -1,  0,  0,  0,  3,  3,  0,  0,  0,  0,  0,  0,  0,  0, 10, 10,  0,  0,  0,  0,  0,  0,  0,  0},
                    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,-20,-50,  0,  0,  0,  0,  1, -1,  0,  0,  1,  5,  8,  0,  0,  0,  0,  0,  0,  0, 10,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0,  2,  2,  2,  1,-20,-20,-30,  0,  0,  0,  0,  1, -1,  0,  0,  2,  5,  9,  0,  0,  0,  0,  0,  0,  0, 10, 10,  0, 10,  0,  0,  0,  0,  0,  0},
                    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,-20,-10,  0,  0,  0,  0,  0,  1, -1,  0,  0,  2,  5,  7,  0,  0,  0,  0,  0,  0, 10, 10, 10,  0, 10,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  4,  4,  2,  3,  2,  1, -5,  0,  0,  0,  0,  0,  0,  1, -1,  0,  0,  2,  4,  0,  0,  0,  0,  0,  0,  0, 10, 10,  0,  0,  0,  0,  0,  0,  0,  0},
                    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 10, 10,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  4,  3,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 10,  0,  0,  0,  0,  0,  0,  0,  0},
                    {0,  0,  0,  0,  0,  0,  0,0.5,  1,  1,  0,  0,  0,  0,  0,  0,-30,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 10, 10,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  4,  4,  2,  3,  1,  1,  1,  0,  0,  0,-30,-30,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 10, 10,  0,  0,  0,  0,  0,  0,  0},          
                    {0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  0,  0,  0,  0,-30,-30,-30,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 10,  0,  0,  0,  0,  0,  0,  0},
					            {0,  0,  0,  0,  0,  5,  4,  2,  2,  1,  1,  1,  0,  0,  0,-30,-30,-25,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 10, 10,  0,  0,  0,  0,  0,  0,  0},  
					          {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,-25,-22,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 10, 10,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,-20,60,-20,  0,  0,  0,  0,  0,  0,  0, 10,  0,-22,-20,  0,  0, -3,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 10, 10,  0,  0,  0,  0,  0,  0},
					          {0,  0,  0,  0,  0,  0, 70,60,-20,  0,  0,  0,  0,  0,  0, 10, 10,  0,-19,-18,  0, -6, -3,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 10, 10,  9,  7,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0, 65,50,  0,  0,  0,  0,  0,  0,  5, 10,  0,  0,-16,-13, -8, -6,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 10, 10,  8,  7,  0,  0,  0,  0},
					          {0,  0,  0,  0,  0,  0,-20,-22,-20,  0,  0,  0,  0,  0,  2,  5,  0,  0,  0,-13,-10, -8,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 10, 10,  8,  7,  7,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0,-20,-20,-15,  0,  0,  0,  0,  0,  2,  0,  0,  0,-10,-13,-10,-10,-12,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 10, 10,  9,  7,  7,  0,  0,  0},
                    {0,  0,  0,  0,  0,  0,  0,-10,-10,-10,  0,  0,  0,  0,  0,  0,  0,  0,  0,-13,-13,-12,-12,-12,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 10, 10,  9,  7,  0,  0,  0},
					            {0,  0,  0,  0,  0,  0,  0,-10,-10,  0,  0,  0,  0,  0,  0,  0,  0,  0,-14,-14,-12,-12,-12,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 10, 10,  9,  7,  0,  0,  0},
                    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,-14,-12,-12,-12,-12,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 10, 10,  9,  0,  0,  0},
					            {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,-14,-12,-12,-12,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 10, 10,  9,  0,  0,  0},
                    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,-14,-12,-12,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 10, 10, 10,  7,  0,  0},
					            {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 10, 10, 10,  0,  0,  0},
                    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 10, 10, 10,  0,  0},
                      {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 10, 10, 10,  0,  0},
                    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 10, 10,  0,  0},
					            {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 10, 10,  0,  0},
                    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 10,  0,  0},
					            {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 10,  0,  0},
                    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10,  0,  0},
					            {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
					            {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}};
                    
   lkolumn = 40;         // o 1 mniej, gdy� kolumna jest r�wnowa�na ci�gowi kwadrat�w 
   lwierszy = sizeof(t)/sizeof(float)/lkolumn/2-1;                 
   mapa = new float*[lwierszy*2+1];
   for (long i=0;i<lwierszy*2+1;i++) {
     mapa[i] = new float[lkolumn+1];
     for (long j=0;j<lkolumn+1;j++) mapa[i][j] = t[i][j];
   }  	
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
       
   fprintf(f,"mapa terenu: lwierszy = %d, lkolumn = %d\n",lwierszy,lkolumn);
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
  //if ((k < 0)||(k >= lwierszy)||(w < 0)||(w >= lkolumn)) return -1e10;  // je�li poza map�

  // korekta numeru kolumny lub wiersza w przypadku terenu cyklicznego
  if (k<0) k += lkolumn;
  else if (k > lkolumn-1) k-=lkolumn;
  if (w<0) w += lwierszy;
  else if (w > lwierszy-1) w-=lwierszy;
  
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
                 
}



void Teren::Rysuj()
{
  glCallList(PowierzchniaTerenu);                  
}

   
