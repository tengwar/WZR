
import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.border.*;

import jade.gui.*;


public class MobileAgentGui extends JFrame {
// -----------------------------------------

   private JTextField location;
   private JTextField info;
   private MobileAgent myAgent;

   public PanelPlanszy plansza = new PanelPlanszy();

   

   public MobileAgentGui(MobileAgent a) {
// --------------------------------------
      plansza.liczba_agentow = 0;
      myAgent = a;
      setTitle(myAgent.getLocalName());

      // Add button and text field
      Container c = getContentPane();
      JPanel base = new JPanel();
      base.setBorder(new EmptyBorder(plansza.rozmiar,20,20,20));
      getContentPane().add(base);
      base.setLayout(new BorderLayout(0,20));

      JPanel pane = new JPanel();
      pane.setLayout(new BorderLayout(20,0));
      pane.add(new JLabel("Current location : "), BorderLayout.WEST);   
      pane.add(location = new JTextField(20), BorderLayout.EAST);
      location.setEditable(false);
      location.setBackground(Color.white);
      
      
      
            
      base.add(plansza, BorderLayout.SOUTH);
      base.add(pane, BorderLayout.AFTER_LAST_LINE);
      base.add(info = new JTextField(25), BorderLayout.SOUTH);
      info.setEditable(false);
      info.setHorizontalAlignment(JTextField.CENTER);

      setSize(900,300);
      pack();
      setResizable(true);
      //this.setBounds(0, 0, 800, 600);
      Rectangle r = getGraphicsConfiguration().getBounds();
      setLocation(r.x + r.width-getWidth(), r.y);
   }
    public void UstawRozmiaryPlanszy(int lwierszy,int lkolumn)
    {
        plansza.liczba_wierszy = lwierszy;
        plansza.liczba_kolumn = lkolumn;
    }

   public void setLocation(String loc){

      this.location.setText(loc);
   }

   public void setInfo(String info){

      this.info.setText(info);
   }

}