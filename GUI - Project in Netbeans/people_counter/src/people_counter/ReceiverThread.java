/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package people_counter;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;

/**
 *
 * @author lucek
 */
public class ReceiverThread extends Thread {
    CountingPlot p;
    String channel;

    public ReceiverThread(CountingPlot p,String channel){
        this.p=p;
        this.channel=channel;
    }

    @Override
    public void run() {
        Runtime ran = Runtime.getRuntime();
    Process proc = null;
    byte[] w = null;
    String path="/home/lucek/tools/airprobe/gsm-receiver/src/python";
    try {
        //String comboCMD = "cd "+path+"; ./gsm_receiveLucek.py -a "+channel+" -c 'D';";
        String comboCMD = "cd "+path+" && whoami";
        String[] cmd = { "/bin/sh", "-c",comboCMD };
        proc = ran.exec(cmd);
        BufferedReader is = new BufferedReader(new InputStreamReader(proc.getInputStream()));
        String line;
        while ((line = is.readLine()) != null) {
            System.out.println(line);
            p.addIdentifier(line);
        }
    } catch (IOException ex) {
    }
    }
}
