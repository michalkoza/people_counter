/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package people_counter;

import java.io.BufferedReader;
import java.util.HashSet;
import java.util.Random;
import java.util.Vector;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.swing.JFrame;
import org.math.plot.Plot2DPanel;

/**
 *
 * @author Lucek
 */
public class CountingPlot extends Thread {

    private Vector<Double> x;
    private Vector<Double> y;
    private int len = 20;
    private int size = 0;
    private long delay;
    private BufferedReader in;
    private HashSet<String> data;
    private String channel;
    
    public CountingPlot(String channel, long delay){
        this.delay=delay;
        x = new Vector<Double>();
        y = new Vector<Double>();
        data = new HashSet<String>();
        this.channel = channel;
    }
    
    public CountingPlot(String channel){
        this.delay=1000;
        x = new Vector<Double>();
        y = new Vector<Double>();
        data = new HashSet<String>();
        this.channel = channel;
    }

    public void addIdentifier(String s){
        data.add(s);
    }

    private void add(double value){
        x.add(x.size(),(double)size);
        y.add(y.size(),value);
        if(x.size()>len){
            x.remove(0);
            y.remove(0);
        }
        size++;
    }
    
    private double[] copyToArray(Vector<Double> x){
        double[] z = new double[x.size()];
        for(int i=0;i<x.size();i++){
            z[i]=x.get(i);
        }
        return z;
    }
    
    private void readFromReceiver(){
        add(data.size());
        data.clear();
    }
    
    @Override
    public void run() {
        ReceiverThread r = new ReceiverThread(this,channel);
        r.start();
        Plot2DPanel plot = new Plot2DPanel();
        plot.setAxisLabel(0, "Time in [s]");
        plot.setAxisLabel(1, "Number of Identifiers");
        JFrame frame = new JFrame("GSM Counter");
        frame.setContentPane(plot);
        frame.setSize(500, 500);
        frame.setVisible(true);
        add(0);
        while(frame.isVisible()){        
            plot.removeAllPlots();
            plot.addLinePlot("", copyToArray(x), copyToArray(y));
            plot.setFixedBounds(0, (float)(double)x.get(0), (float)((double)x.get(0)+len+2));
            try {
                Thread.sleep(delay);
            } catch (InterruptedException ex) {
                Logger.getLogger(CountingPlot.class.getName()).log(Level.SEVERE, null, ex);
            }
            plot.repaint();
            readFromReceiver();
        }
        //r.stop();
    }
    
}
