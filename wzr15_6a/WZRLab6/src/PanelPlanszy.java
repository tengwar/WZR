/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

/**
 *
 * @author J
 */

import java.awt.BasicStroke;
import java.awt.Color;
import java.awt.Graphics;
import java.awt.Graphics2D;
import javax.swing.JPanel;

public class PanelPlanszy extends JPanel{// implements Runnable{

    // private final int PROMIEN=80;
    int rozmiar = 400;

    int pola[][]={};
    int liczba_wierszy=0;
    int liczba_kolumn=0;

    StanInagroda moj_stan = new StanInagroda();   // stan agenta (numery wiersza i kolumny)
    int moja_akcja;   // akcja agenta w tym stanie
    int moj_numer;

    StanInagroda stany_agentow[] = new StanInagroda[100]; // stany wszystkich agentow
    int liczba_agentow = 0;

    String tekst1 = "", tekst2 = "";

    public PanelPlanszy() // Konstruktor
    {
        setSize(rozmiar,rozmiar);
        /*Thread watek1=new Thread(this);
        watek1.start();
        Watek2 watek2=new Watek2();
        watek2.start();*/
    }

    public void paintComponent(Graphics g)  // metoda odrysowująca panel
    {
        Graphics2D g2=(Graphics2D)g;

        g2.clearRect(0, 0, rozmiar, rozmiar);
        //Stroke kreska=g2.getStroke();

        // Rysowanie planszy z agentem (niebieski kwadrat) oraz innymi agentami
        // (zielone kwadraty)
        int rozm_pola = rozmiar/(liczba_wierszy > liczba_kolumn ? liczba_wierszy : liczba_kolumn);
        int m = rozm_pola/6; // wielkosc marginesow dla obiektow na planszy
        for (int i=0;i<liczba_wierszy;i++)
            for (int j=0;j<liczba_kolumn;j++)
            {
                g2.setColor(new Color(255,255,255));
                g2.setColor(new Color(120,120,120));
                g2.setStroke(new BasicStroke(2.0f,BasicStroke.CAP_ROUND,BasicStroke.JOIN_MITER) );
                g2.drawRect(j*rozm_pola, i*rozm_pola, rozm_pola, rozm_pola);
                if (pola[i][j]==1)  // sciany
                {
                  g2.setColor(new Color(0,0,0));
                  g2.fillRect(j*rozm_pola, i*rozm_pola, rozm_pola, rozm_pola);
                }
                else if (pola[i][j]==2) // nagrody
                {
                  g2.setColor(new Color(255,255,0));
                  g2.fillOval(j*rozm_pola+m, i*rozm_pola+m, rozm_pola-2*m, rozm_pola-2*m);
                }
                else if (pola[i][j]==3) // pulapki
                {
                  g2.setColor(new Color(255,0,0));
                  g2.fillOval(j*rozm_pola+m, i*rozm_pola+m, rozm_pola-2*m, rozm_pola-2*m);
                }
                else if (pola[i][j]==4) // bieguny
                {
                  g2.setColor(new Color(128,0,128));
                  g2.drawArc(j*rozm_pola+m, i*rozm_pola+m, rozm_pola/2, rozm_pola/2, 0,360);
                }
            }

        g2.setColor(new Color(0,0,255));

        g2.setColor(new Color(0,0,255));
        g2.fillRect(moj_stan.k*rozm_pola+m, moj_stan.w*rozm_pola+m, rozm_pola-2*m, rozm_pola-2*m);  // własny obiekt
        g2.setColor(new Color(255,255,255));
        String st = new String(""+moj_numer);
        g2.drawString(st, moj_stan.k*rozm_pola+m*3,  moj_stan.w*rozm_pola+m*4);
        // akcja jaką próbuje wykonać agent w tym stanie:
        if (moja_akcja > 0)
        {
            //System.exit(1);
            g2.setColor(new Color(0,140,255));
            if (moja_akcja == 1)
                g2.fillOval(moj_stan.k*rozm_pola+rozm_pola-m, moj_stan.w*rozm_pola+rozm_pola/3, m*2, m*2);
            else if (moja_akcja == 2)
                g2.fillOval(moj_stan.k*rozm_pola+rozm_pola/3, moj_stan.w*rozm_pola+rozm_pola-m, m*2, m*2);
            else if (moja_akcja == 3)
                g2.fillOval(moj_stan.k*rozm_pola-m, moj_stan.w*rozm_pola+rozm_pola/3, m*2, m*2);
            else if (moja_akcja == 4)
                g2.fillOval(moj_stan.k*rozm_pola+rozm_pola/3, moj_stan.w*rozm_pola-m, m*2, m*2);

        }


        for (int i=0;i<liczba_agentow;i++)
            if (i != moj_numer)
            {
               g2.setColor(new Color(0,192,0));
               g2.fillRect(stany_agentow[i].k *rozm_pola+m, stany_agentow[i].w*rozm_pola+m, rozm_pola-2*m, rozm_pola-2*m);
               g2.setColor(new Color(255,255,255));
               st = new String(""+i);
               g2.drawString(st, stany_agentow[i].k*rozm_pola+m*3, stany_agentow[i].w*rozm_pola+m*4);
            }
        g2.setColor(new Color(0,0,0));
        g2.drawString(tekst1,10,(liczba_wierszy+1)*rozm_pola);
        g2.drawString(tekst2,10,(liczba_wierszy+2)*rozm_pola);
    }

    /*public void run()
    {
        while(true)
        {  
            try
            {
                Thread.sleep(100);
            }
            catch(Exception ek){}
            repaint();
        }//koniec while
    } //koniec metody run()
    */

    /*class Watek2 extends Thread
    {
        public void run()
        {
            while(true)
            {
                Random losuj=new Random();
                kr=losuj.nextInt(255);
                kg=losuj.nextInt(255);
                kb=losuj.nextInt(255);
                try
                {
                    Thread.sleep(15000);
                    //System.out.println("pola[2][2] = " + pola[2][2]);
                }
                catch(Exception e){}
                repaint();
            }//koniec while
        }//koniec metody run()
    }//koniec klasy wew

     */
} // koniec klasy PanelPlanszy