/*********************************************************************
    Symulacja obiektów fizycznych ruchomych np. samochody, statki, roboty, itd. 
    + obs³uga obiektów statycznych np. teren.
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

	F = Fb = alfa  = 0;	// si³y dzia³aj¹ce na obiekt 
	ham = 0;			// stopieñ hamowania
	m = 5.0;			// masa obiektu [kg]
	Fy = m*9.81;        // si³a nacisku na podstawê obiektu (na ko³a pojazdu)
	dlugosc = 10.0;
	szerokosc = 4.0;
	wysokosc = 1.6;
	przeswit = 0.0;     // wysokoœæ na której znajduje siê podstawa obiektu
	dl_przod = 1.0;     // odleg³oœæ od przedniej osi do przedniego zderzaka 
	dl_tyl = 0.2;       // odleg³oœæ od tylniej osi do tylniego zderzaka
	
  wPol.y = przeswit+wysokosc/2 + 25; // wysokoœæ œrodka ciê¿koœci w osi pionowej pojazdu
  wPol.x = 0;
  wPol.z = 0;

	// obrót obiektu o k¹t 30 stopni wzglêdem osi y:
	kwaternion qObr = AsixToQuat(Wektor3(0,1,0),0.1*PI/180.0);
	qOrient = qObr*qOrient;
}

ObiektRuchomy::~ObiektRuchomy()            // destruktor
{
}

void ObiektRuchomy::ZmienStan(StanObiektu stan)  // przepisanie podanego stanu 
{                                                // w przypadku obiektów, które nie s¹ symulowane
	this->iID = stan.iID;                        

	this->wPol = stan.wPol;
	this->qOrient = stan.qOrient;
	this->wV = stan.wV;
	this->wA = stan.wA;
	this->wV_kat = stan.wV_kat;
	this->wA_kat = stan.wA_kat;
}

StanObiektu ObiektRuchomy::Stan()                // metoda zwracaj¹ca stan obiektu ³¹cznie z iID
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
{                                                // dzia³aj¹cych si³ i czasu, jaki up³yn¹³ od ostatniej symulacji

  if (dt == 0) return;

  float tarcie = 3.0;            // wspó³czynnik tarcia obiektu o pod³o¿e 
	float tarcie_obr = tarcie;     // tarcie obrotowe (w szczególnych przypadkach mo¿e byæ inne ni¿ liniowe)
  float tarcie_toczne = 0.08;    // wspó³czynnik tarcia tocznego
  float sprezystosc = 0.5;       // wspó³czynnik sprê¿ystoœci (0-brak sprê¿ystoœci, 1-doskona³a sprê¿ystoœæ) 
  float g = 9.81;                // przyspieszenie grawitacyjne
       
  // obracam uk³ad wspó³rzêdnych lokalnych wed³ug kwaterniona orientacji:
	Wektor3 w_przod = qOrient.obroc_wektor(Wektor3(1,0,0)); // na razie oœ obiektu pokrywa siê z osi¹ x globalnego uk³adu wspó³rzêdnych (lokalna oœ x)
	Wektor3 w_gora = qOrient.obroc_wektor(Wektor3(0,1,0));  // wektor skierowany pionowo w górê od podstawy obiektu (lokalna oœ y)
	Wektor3 w_prawo = qOrient.obroc_wektor(Wektor3(0,0,1)); // wektor skierowany w prawo (lokalna oœ z)
	

	// rzutujemy wV na sk³adow¹ w kierunku przodu i pozosta³e 2 sk³adowe
	// sk³adowa w bok jest zmniejszana przez si³ê tarcia, sk³adowa do przodu
	// przez si³ê tarcia tocznego
	Wektor3 wV_wprzod = w_przod*(wV^w_przod),
	        wV_wprawo = w_prawo*(wV^w_prawo),
	        wV_wgore = w_gora*(wV^w_gora); 
	        
    // rzutujemy prêdkoœæ k¹tow¹ wV_kat na sk³adow¹ w kierunku przodu i pozosta³e 2 sk³adowe
	Wektor3 wV_kat_wprzod = w_przod*(wV_kat^w_przod),
	        wV_kat_wprawo = w_prawo*(wV_kat^w_prawo),
	        wV_kat_wgore = w_gora*(wV_kat^w_gora);         


    // ograniczenia 
	if (alfa > PI*45.0/180) alfa = PI*45.0/180 ;
	if (alfa < -PI*45.0/180) alfa = -PI*45.0/180 ;

    // obliczam promien skrêtu pojazdu na podstawie k¹ta skrêtu kó³, a nastêpnie na podstawie promienia skrêtu
	// obliczam prêdkoœæ k¹tow¹ (UPROSZCZENIE! pomijam przyspieszenie k¹towe oraz w³aœciw¹ trajektoriê ruchu)
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
		if (wV_kat_wgore2.dlugosc() <= wV_kat_wgore.dlugosc()) // skrêt przeciwdzia³a obrotowi
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

    // tarcie zmniejsza prêdkoœæ obrotow¹ (UPROSZCZENIE! zamiast masy winienem wykorzystaæ moment bezw³adnoœci)     
    float V_kat_tarcie = Fy*tarcie_obr*dt/m/1.0;      // zmiana pr. k¹towej spowodowana tarciem
    float V_kat_wgore = wV_kat_wgore.dlugosc() - V_kat_tarcie;
    if (V_kat_wgore < V_kat_skret) V_kat_wgore = V_kat_skret;        // tarcie nie mo¿e spowodowaæ zmiany zwrotu wektora pr. k¹towej
    wV_kat_wgore = wV_kat_wgore.znorm()*V_kat_wgore;                     
  }    
    
                  
  Fy = m*g*w_gora.y;                      // si³a docisku do pod³o¿a 
  if (Fy < 0 ) Fy = 0;
             // ... trzeba j¹ jeszcze uzale¿niæ od tego, czy obiekt styka siê z pod³o¿em!
  float Fh = Fy*tarcie*ham;                  // si³a hamowania (UP: bez uwzglêdnienia poœlizgu)
    
  float V_wprzod = wV_wprzod.dlugosc();// - dt*Fh/m - dt*tarcie_toczne*Fy/m;
  if (V_wprzod < 0) V_wprzod = 0;
    
  float V_wprawo = wV_wprawo.dlugosc();// - dt*tarcie*Fy/m;
  if (V_wprawo < 0) V_wprawo = 0;
      
      
  // wjazd lub zjazd: 
  //wPol.y = teren.Wysokosc(wPol.x,wPol.z);   // najprostsze rozwi¹zanie - obiekt zmienia wysokoœæ bez zmiany orientacji
	
	// 1. gdy wjazd na wklês³oœæ: wyznaczam wysokoœci terenu pod naro¿nikami obiektu (ko³ami), 
    // sprawdzam która trójka
	// naro¿ników odpowiada najni¿ej po³o¿onemu œrodkowi ciê¿koœci, gdy przylega do terenu
	// wyznaczam prêdkoœæ podbicia (wznoszenia œrodka pojazdu spowodowanego wklês³oœci¹) 
    // oraz prêdkoœæ k¹tow¹
	// 2. gdy wjazd na wypuk³oœæ to si³a ciê¿koœci wywo³uje obrót przy du¿ej prêdkoœci liniowej

	// punkty zaczepienia kó³ (na wysokoœci pod³ogi pojazdu):
  Wektor3 P = wPol + w_przod*(dlugosc/2-dl_przod) - w_prawo*szerokosc/2 - w_gora*wysokosc/2,
          Q = wPol + w_przod*(dlugosc/2-dl_przod) + w_prawo*szerokosc/2 - w_gora*wysokosc/2,
			    R = wPol + w_przod*(-dlugosc/2+dl_tyl) - w_prawo*szerokosc/2 - w_gora*wysokosc/2,
			    S = wPol + w_przod*(-dlugosc/2+dl_tyl) + w_prawo*szerokosc/2 - w_gora*wysokosc/2;

    // pionowe rzuty punktów zacz. kó³ pojazdu na powierzchniê terenu:  
	Wektor3 Pt = P, Qt = Q, Rt = R, St = S;
  Pt.y = teren.Wysokosc(P.x,P.z); Qt.y = teren.Wysokosc(Q.x,Q.z);  
  Rt.y = teren.Wysokosc(R.x,R.z); St.y = teren.Wysokosc(S.x,S.z);   
  Wektor3 normPQR = normalna(Pt,Rt,Qt), normPRS = normalna(Pt,Rt,St), normPQS = normalna(Pt,St,Qt),
          normQRS = normalna(Qt,Rt,St);   // normalne do p³aszczyzn wyznaczonych przez trójk¹ty
           	
	fprintf(f,"P.y = %f, Pt.y = %f, Q.y = %f, Qt.y = %f, R.y = %f, Rt.y = %f, S.y = %f, St.y = %f\n",
		        P.y, Pt.y, Q.y, Qt.y, R.y,Rt.y, S.y, St.y);
	
  float sryPQR = ((Qt^normPQR) - normPQR.x*wPol.x - normPQR.z*wPol.z)/normPQR.y, // wys. œrodka pojazdu
        sryPRS = ((Pt^normPRS) - normPRS.x*wPol.x - normPRS.z*wPol.z)/normPRS.y, // po najechaniu na skarpê 
        sryPQS = ((Pt^normPQS) - normPQS.x*wPol.x - normPQS.z*wPol.z)/normPQS.y, // dla 4 trójek kó³
        sryQRS = ((Qt^normQRS) - normQRS.x*wPol.x - normQRS.z*wPol.z)/normQRS.y;
  float sry = sryPQR; Wektor3 norm = normPQR;
  if (sry > sryPRS) {sry = sryPRS; norm = normPRS;}  
  if (sry > sryPQS) {sry = sryPQS; norm = normPQS;}
  if (sry > sryQRS) {sry = sryQRS; norm = normQRS;}  // wybór trójk¹ta o œrodku najni¿ej po³o¿onym    
           
  Wektor3 wV_kat_wpoziomie = Wektor3(0,0,0);
  // jesli któreœ z kó³ jest poni¿ej powierzchni terenu
  if ((P.y <= Pt.y + wysokosc/2+przeswit)||(Q.y <= Qt.y  + wysokosc/2+przeswit)||
      (R.y <= Rt.y  + wysokosc/2+przeswit)||(S.y <= St.y  + wysokosc/2+przeswit))
  {   
     // obliczam powsta³¹ prêdkoœæ k¹tow¹ w lokalnym uk³adzie wspó³rzêdnych:      
     Wektor3 wobrot = -norm.znorm()*w_gora*0.6;  
     wV_kat_wpoziomie = wobrot/dt; 
  }    

  Wektor3 wAg = Wektor3(0,-1,0)*g;    // przyspieszenie grawitacyjne

	// jesli wiecej niz 2 kola sa na ziemi, to przyspieszenie grawitacyjne jest rownowazone przez opor gruntu:
  if ((P.y <= Pt.y + wysokosc/2+przeswit)+(Q.y <= Qt.y  + wysokosc/2+przeswit)+
      (R.y <= Rt.y  + wysokosc/2+przeswit)+(S.y <= St.y  + wysokosc/2+przeswit) > 2)
    wAg = wAg + w_gora*(w_gora^wAg)*-1; //przyspieszenie wynikaj¹ce z si³y oporu gruntu
	else   // w przeciwnym wypadku brak sily docisku 
		Fy = 0;
	

  // sk³adam z powrotem wektor prêdkoœci k¹towej: 
  //wV_kat = wV_kat_wgore + wV_kat_wprawo + wV_kat_wprzod;  
  wV_kat = wV_kat_wgore + wV_kat_wpoziomie;
      
      
  float h = sry+wysokosc/2+przeswit - wPol.y;  // ró¿nica wysokoœci jak¹ trzeba pokonaæ  
  float V_podbicia = 0;
  if ((h > 0)&&(wV.y <= 0.01))
    V_podbicia = 0.5*sqrt(2*g*h);  // prêdkoœæ spowodowana podbiciem pojazdu przy wje¿d¿aniu na skarpê 
  if (h > 0) wPol.y = sry+wysokosc/2+przeswit;  
      
  // lub  w przypadku zag³êbienia siê 
  Wektor3 dwPol = wV*dt + wA*dt*dt/2; // czynnik bardzo ma³y - im wiêksza czêstotliwoœæ symulacji, tym mniejsze znaczenie 
  wPol = wPol + dwPol;  
  
  // korekta po³o¿enia w przypadku terenu cyklicznego:
  if (wPol.x < -teren.rozmiar_pola*teren.lkolumn/2) wPol.x += teren.rozmiar_pola*teren.lkolumn;
  else if (wPol.x > teren.rozmiar_pola*(teren.lkolumn-teren.lkolumn/2)) wPol.x -= teren.rozmiar_pola*teren.lkolumn;
  if (wPol.z < -teren.rozmiar_pola*teren.lwierszy/2) wPol.z += teren.rozmiar_pola*teren.lwierszy;
  else if (wPol.z > teren.rozmiar_pola*(teren.lwierszy-teren.lwierszy/2)) wPol.z -= teren.rozmiar_pola*teren.lwierszy;

  // Sprawdzenie czy obiekt mo¿e siê przemieœciæ w zadane miejsce: Jeœli nie, to 
	// przemieszczam obiekt do miejsca zetkniêcia, wyznaczam nowe wektory prêdkoœci
	// i prêdkoœci k¹towej, a nastêpne obliczam nowe po³o¿enie na podstawie nowych
	// prêdkoœci i pozosta³ego czasu. Wszystko powtarzam w pêtli (pojazd znowu mo¿e 
	// wjechaæ na przeszkodê). Problem z zaokr¹glonymi przeszkodami - konieczne 
	// wyznaczenie minimalnego kroku.
      
      
  Wektor3 wV_pop = wV;  

  // sk³adam prêdkoœci w ró¿nych kierunkach oraz efekt przyspieszenia w jeden wektor:    (problem z przyspieszeniem od si³y tarcia -> to przyspieszenie 
	//      mo¿e dzia³aæ krócej ni¿ dt -> trzeba to jakoœ uwzglêdniæ, inaczej pojazd bêdzie wê¿ykowa³)
	wV = wV_wprzod.znorm()*V_wprzod + wV_wprawo.znorm()*V_wprawo + wV_wgore + 
         Wektor3(0,1,0)*V_podbicia + wA*dt;
	// usuwam te sk³adowe wektora prêdkoœci w których kierunku jazda nie jest mo¿liwa z powodu
	// przeskód:
	// np. jeœli pojazd styka siê 3 ko³ami z nawierzchni¹ lub dwoma ko³ami i œrodkiem ciê¿koœci to
	// nie mo¿e mieæ prêdkoœci w dó³ pod³ogi
	if ((P.y <= Pt.y  + wysokosc/2+przeswit)||(Q.y <= Qt.y  + wysokosc/2+przeswit)||  
        (R.y <= Rt.y  + wysokosc/2+przeswit)||(S.y <= St.y  + wysokosc/2+przeswit))    // jeœli pojazd styka siê co najm. jednym ko³em
	{
	   Wektor3 dwV = wV_wgore + w_gora*(wA^w_gora)*dt;
	   if ((w_gora.znorm() - dwV.znorm()).dlugosc() > 1 )  // jeœli wektor skierowany w dó³ pod³ogi
	    	wV = wV - dwV;
	}
	 
	// sk³adam przyspieszenia liniowe od si³ napêdzaj¹cych i od si³ oporu: 
	wA = (w_przod*F + w_prawo*Fb)/m*(Fy>0)  // od si³ napêdzaj¹cych
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
	
	Wektor3 k = qOrient.AsixAngle();     // reprezentacja k¹towo-osiowa kwaterniona
	
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
  rozmiar_pola = 40;         // d³ugoœæ boku kwadratu w [m]           
  float t[][41] = { {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                      {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // ostatni element nieu¿ywany
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
                    
   lkolumn = 40;         // o 1 mniej, gdy¿ kolumna jest równowa¿na ci¹gowi kwadratów 
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

float Teren::Wysokosc(float x,float z)      // okreœlanie wysokoœci dla punktu o wsp. (x,z) 
{
  
  float pocz_x = -rozmiar_pola*lkolumn/2,     // wspó³rzêdne lewego górnego krañca terenu
        pocz_z = -rozmiar_pola*lwierszy/2;        
  
  long k = (long)((x - pocz_x)/rozmiar_pola), // wyznaczenie wspó³rzêdnych (w,k) kwadratu
       w = (long)((z - pocz_z)/rozmiar_pola);
  //if ((k < 0)||(k >= lwierszy)||(w < 0)||(w >= lkolumn)) return -1e10;  // jeœli poza map¹

  // korekta numeru kolumny lub wiersza w przypadku terenu cyklicznego
  if (k<0) k += lkolumn;
  else if (k > lkolumn-1) k-=lkolumn;
  if (w<0) w += lwierszy;
  else if (w > lwierszy-1) w-=lwierszy;
  
  // wyznaczam punkt B - œrodek kwadratu oraz trójk¹t, w którym znajduje siê punkt
  // (rysunek w Teren::PoczatekGrafiki())
  Wektor3 B = Wektor3(pocz_x + (k+0.5)*rozmiar_pola, mapa[w*2+1][k], pocz_z + (w+0.5)*rozmiar_pola); 
  enum tr{ABC=0,ADB=1,BDE=2,CBE=3};       // trójk¹t w którym znajduje siê punkt 
  int trojkat=0; 
  if ((B.x > x)&&(fabs(B.z - z) < fabs(B.x - x))) trojkat = ADB;
  else if ((B.x < x)&&(fabs(B.z - z) < fabs(B.x - x))) trojkat = CBE;
  else if ((B.z > z)&&(fabs(B.z - z) > fabs(B.x - x))) trojkat = ABC;
  else trojkat = BDE;
  
  // wyznaczam normaln¹ do p³aszczyzny a nastêpnie wspó³czynnik d z równania p³aszczyzny
  float dd = d[w][k][trojkat];
  Wektor3 N = Norm[w][k][trojkat];
  float y;
  if (N.y > 0) y = (-dd - N.x*x - N.z*z)/N.y;
  else y = 0;
  
  return y;    
}

void Teren::PoczatekGrafiki()
{
  // tworze listê wyœwietlania rysuj¹c poszczególne pola mapy za pomoc¹ trójk¹tów 
  // (po 4 trójk¹ty na ka¿de pole):
  enum tr{ABC=0,ADB=1,BDE=2,CBE=3};       
  float pocz_x = -rozmiar_pola*lkolumn/2,     // wspó³rzêdne lewego górnego krañca terenu
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
          // tworzê trójk¹t ABC w górnej czêœci kwadratu: 
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
          d[w][k][ABC] = -(B^N);          // dodatkowo wyznaczam wyraz wolny z równania plaszyzny trójk¹ta
          Norm[w][k][ABC] = N;          // dodatkowo zapisujê normaln¹ do p³aszczyzny trójk¹ta
          // trójk¹t ADB:
          Wektor3 AD = D-A;
          N = (AD*AB).znorm();          
          glNormal3f( N.x, N.y, N.z);
		  glVertex3f( A.x, A.y, A.z);
		  glVertex3f( D.x, D.y, D.z);
		  glVertex3f( B.x, B.y, B.z);
		  d[w][k][ADB] = -(B^N);       
          Norm[w][k][ADB] = N;
		  // trójk¹t BDE:
          Wektor3 BD = D-B;
          Wektor3 DE = E-D;
          N = (BD*DE).znorm();          
          glNormal3f( N.x, N.y, N.z);
		  glVertex3f( B.x, B.y, B.z);
          glVertex3f( D.x, D.y, D.z);     
          glVertex3f( E.x, E.y, E.z);  
          d[w][k][BDE] = -(B^N);        
          Norm[w][k][BDE] = N;  
          // trójk¹t CBE:
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

   
