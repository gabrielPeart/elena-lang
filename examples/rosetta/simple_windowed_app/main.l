import forms;
import extensions;

public class MainWindow : SDIDialog
{
    Label  lblClicks;
    Button btmClickMe;
    
    //Store how much clicks the user doed
    int clicksCount;
    
    constructor new()
       <= new()
    {        
        lblClicks := new Label();
        btmClickMe := new Button();
        
        clicksCount := 0;
        self
            .appendControl(lblClicks)
            .appendControl(btmClickMe);        
    
        self.Caption := "Rosseta Code";
        self.setRegion(100, 100, 160, 80);
    
        lblClicks.Caption := "Clicks: 0";
        lblClicks.setRegion(10, 2, 160, 20);

        btmClickMe.Caption := "Click me";
        btmClickMe.setRegion(7, 20, 140, 30);
        
        btmClickMe.onClick := (args){ self.onButtonClick(); };
    }
    
    private onButtonClick()
    {
        clicksCount := clicksCount + 1;
        
        lblClicks.Caption := "Clicks: " + clicksCount.toString();
    }
}