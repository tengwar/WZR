/****************************************************
    Wirtualne zespoly robocze - przykladowy projekt w C++
    Do zadañ dotycz¹cych wspó³pracy, ekstrapolacji i 
    autonomicznych obiektów
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
	WNDCLASS nasza_klasa; //klasa g³ównego okna aplikacji

	static char nazwa_klasy[] = "Podstawowa";

	//Definiujemy klase g³ównego okna aplikacji
	//Okreslamy tu wlasciwosci okna, szczegoly wygladu oraz
	//adres funkcji przetwarzajacej komunikaty
	nasza_klasa.style         = CS_HREDRAW | CS_VREDRAW;
	nasza_klasa.lpfnWndProc   = WndProc; //adres funkcji realizuj¹cej przetwarzanie meldunków 
 	nasza_klasa.cbClsExtra    = 0 ;
	nasza_klasa.cbWndExtra    = 0 ;
	nasza_klasa.hInstance     = hInstance; //identyfikator procesu przekazany przez MS Windows podczas uruchamiania programu
	nasza_klasa.hIcon         = 0;
	nasza_klasa.hCursor       = LoadCursor(0, IDC_ARROW);
	nasza_klasa.hbrBackground = (HBRUSH) GetStockObject(GRAY_BRUSH);
	nasza_klasa.lpszMenuName  = "Menu" ;
	nasza_klasa.lpszClassName = nazwa_klasy;

    //teraz rejestrujemy klasê okna g³ównego
    RegisterClass (&nasza_klasa);
	
	/*tworzymy okno g³ówne
	okno bêdzie mia³o zmienne rozmiary, listwê z tytu³em, menu systemowym
	i przyciskami do zwijania do ikony i rozwijania na ca³y ekran, po utworzeniu
	bêdzie widoczne na ekranie */
 	okno = CreateWindow(nazwa_klasy, "WZR-lab 2015, temat 3, wersja a     [F1-pomoc]", WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
						100, 50, 700, 700, NULL, NULL, hInstance, NULL);
	
	
	ShowWindow (okno, nCmdShow) ;
    
	//odswiezamy zawartosc okna
	UpdateWindow (okno) ;

	// G£ÓWNA PÊTLA PROGRAMU
	
     /* pobranie komunikatu z kolejki; funkcja GetMessage zwraca FALSE tylko dla
	 komunikatu wm_Quit; dla wszystkich pozosta³ych komunikatów zwraca wartoœæ TRUE */
	while(GetMessage(&meldunek, NULL, 0, 0))
	{
		TranslateMessage(&meldunek); // wstêpna obróbka komunikatu
		DispatchMessage(&meldunek);  // przekazanie komunikatu w³aœciwemu adresatowi (czyli funkcji obslugujacej odpowiednie okno)
	}

	return (int)meldunek.wParam;
}

/********************************************************************
FUNKCJA OKNA realizujaca przetwarzanie meldunków kierowanych do okna aplikacji*/
LRESULT CALLBACK WndProc (HWND okno, UINT kod_meldunku, WPARAM wParam, LPARAM lParam)
{
	    	
    // PONI¯SZA INSTRUKCJA DEFINIUJE REAKCJE APLIKACJI NA POSZCZEGÓLNE MELDUNKI 
	KlawiszologiaSterowania(kod_meldunku, wParam, lParam);

	switch (kod_meldunku) 
	{
	case WM_CREATE:  //meldunek wysy³any w momencie tworzenia okna
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
      case VK_ESCAPE:   // wyjœcie z programu
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
  	
	case WM_DESTROY: //obowi¹zkowa obs³uga meldunku o zamkniêciu okna
    if (lParam == 100)
      MessageBox(okno,"Jest zbyt póŸno na do³¹czenie do wirtualnego œwiata. Trzeba to zrobiæ zanim inni uczestnicy zmieni¹ jego stan.","Zamkniêcie programu",MB_OK);  

		ZakonczenieInterakcji();

		ZakonczenieGrafiki();


		ReleaseDC(okno, g_context);
		KillTimer(okno, 1);



		PostQuitMessage (0) ;
		return 0;
    
	default: //standardowa obs³uga pozosta³ych meldunków
		return DefWindowProc(okno, kod_meldunku, wParam, lParam);
	}
	
	
}

