/****************************************************
    Wirtualne zespoly robocze - przykladowy projekt w C++
    Do zada� dotycz�cych wsp�pracy, ekstrapolacji i 
    autonomicznych obiekt�w
 ****************************************************/

#include <windows.h>
#include <math.h>
#include <time.h>

#include <gl\gl.h>
#include <gl\glu.h>

#include "obiekty.h"
#include "grafika.h"
#include "interakcja.h"


//deklaracja funkcji obslugi okna
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);


HWND okno;                   // uchwyt do okna aplikacji
HDC g_context = NULL;        // uchwyt kontekstu graficznego



//funkcja Main - dla Windows
int WINAPI WinMain(HINSTANCE hInstance,
               HINSTANCE hPrevInstance,
               LPSTR     lpCmdLine,
               int       nCmdShow)
{
	MSG meldunek;		  //innymi slowy "komunikat"
	WNDCLASS nasza_klasa; //klasa g��wnego okna aplikacji

	static char nazwa_klasy[] = "Podstawowa";

	//Definiujemy klase g��wnego okna aplikacji
	//Okreslamy tu wlasciwosci okna, szczegoly wygladu oraz
	//adres funkcji przetwarzajacej komunikaty
	nasza_klasa.style         = CS_HREDRAW | CS_VREDRAW;
	nasza_klasa.lpfnWndProc   = WndProc; //adres funkcji realizuj�cej przetwarzanie meldunk�w 
 	nasza_klasa.cbClsExtra    = 0 ;
	nasza_klasa.cbWndExtra    = 0 ;
	nasza_klasa.hInstance     = hInstance; //identyfikator procesu przekazany przez MS Windows podczas uruchamiania programu
	nasza_klasa.hIcon         = 0;
	nasza_klasa.hCursor       = LoadCursor(0, IDC_ARROW);
	nasza_klasa.hbrBackground = (HBRUSH) GetStockObject(GRAY_BRUSH);
	nasza_klasa.lpszMenuName  = "Menu" ;
	nasza_klasa.lpszClassName = nazwa_klasy;

    //teraz rejestrujemy klas� okna g��wnego
    RegisterClass (&nasza_klasa);
	
	/*tworzymy okno g��wne
	okno b�dzie mia�o zmienne rozmiary, listw� z tytu�em, menu systemowym
	i przyciskami do zwijania do ikony i rozwijania na ca�y ekran, po utworzeniu
	b�dzie widoczne na ekranie */
 	okno = CreateWindow(nazwa_klasy, "WZR-lab 2015, temat 3, wersja a     [F1-pomoc]", WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
						100, 50, 700, 700, NULL, NULL, hInstance, NULL);
	
	
	ShowWindow (okno, nCmdShow) ;
    
	//odswiezamy zawartosc okna
	UpdateWindow (okno) ;

	// G��WNA P�TLA PROGRAMU
	
     /* pobranie komunikatu z kolejki; funkcja GetMessage zwraca FALSE tylko dla
	 komunikatu wm_Quit; dla wszystkich pozosta�ych komunikat�w zwraca warto�� TRUE */
	while(GetMessage(&meldunek, NULL, 0, 0))
	{
		TranslateMessage(&meldunek); // wst�pna obr�bka komunikatu
		DispatchMessage(&meldunek);  // przekazanie komunikatu w�a�ciwemu adresatowi (czyli funkcji obslugujacej odpowiednie okno)
	}

	return (int)meldunek.wParam;
}

/********************************************************************
FUNKCJA OKNA realizujaca przetwarzanie meldunk�w kierowanych do okna aplikacji*/
LRESULT CALLBACK WndProc (HWND okno, UINT kod_meldunku, WPARAM wParam, LPARAM lParam)
{
	    	
    // PONI�SZA INSTRUKCJA DEFINIUJE REAKCJE APLIKACJI NA POSZCZEG�LNE MELDUNKI 
	KlawiszologiaSterowania(kod_meldunku, wParam, lParam);

	switch (kod_meldunku) 
	{
	case WM_CREATE:  //meldunek wysy�any w momencie tworzenia okna
		{
			
			g_context = GetDC(okno);

			srand( (unsigned)time( NULL ) );
            int wynik = InicjujGrafike(g_context);
			if (wynik == 0)
			{
				printf("nie udalo sie otworzyc okna graficznego\n");
				//exit(1);
			}

			PoczatekInterakcji();

			SetTimer(okno, 1, 10, NULL);
						
			return 0;
		}
	case WM_KEYDOWN:
    {
      switch (LOWORD(wParam))
      {
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
      case VK_ESCAPE:   // wyj�cie z programu
        {
          SendMessage(okno, WM_DESTROY,0,0);
          break;
        }
      }
      return 0;
    }
       
	case WM_PAINT:
		{
			PAINTSTRUCT paint;
			HDC kontekst;
			kontekst = BeginPaint(okno, &paint);
		
			RysujScene();			
			SwapBuffers(kontekst);

			EndPaint(okno, &paint);

			

			return 0;
		}

	case WM_TIMER:
       Cykl_WS();
	     InvalidateRect(okno, NULL, FALSE);

	    return 0;

  case WM_SIZE:
		{
			int cx = LOWORD(lParam);
			int cy = HIWORD(lParam);

			ZmianaRozmiaruOkna(cx,cy);
			
			return 0;
		}
  	
	case WM_DESTROY: //obowi�zkowa obs�uga meldunku o zamkni�ciu okna
    if (lParam == 100)
      MessageBox(okno,"Jest zbyt p�no na do��czenie do wirtualnego �wiata. Trzeba to zrobi� zanim inni uczestnicy zmieni� jego stan.","Zamkni�cie programu",MB_OK);  

		ZakonczenieInterakcji();

		ZakonczenieGrafiki();


		ReleaseDC(okno, g_context);
		KillTimer(okno, 1);



		PostQuitMessage (0) ;
		return 0;
    
	default: //standardowa obs�uga pozosta�ych meldunk�w
		return DefWindowProc(okno, kod_meldunku, wParam, lParam);
	}
	
	
}

