/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
import static org.junit.Assert.*;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;

import org.junit.Test;

import vnmr.apt.ReflectionData;


public class ReflectionDataTest extends ReflectionData{
    private static String data = "";

    static {
        //URL url = ReflectionDataTest.class.getResource("data");
        //System.out.println("url=" + url);/*CMP*/
        InputStream in
            = ReflectionDataTest.class.getResourceAsStream("testData");
        if (in != null) {
            BufferedReader read = new BufferedReader(new InputStreamReader(in));
            try {
                data = read.readLine();
            } catch (IOException e) {
            }
        }
    }

    public ReflectionDataTest() {
        super(data);
        //scaleToMaxAbsval();
    }

    @Test
    public void setupTest() {
        assertFalse(data.equals(""));
    }

    @Test
    public void dataStatsTest() {
        int gain = getGain();
        int power = getPower();
        double noise = 50 / Math.pow(10, (power + gain + 50) / 20.0);
        assertEquals(noise, getBaselineNoise(), noise / 10);

        // NB: About 200 points are near the baseline level
        double minMax = 1 + noise * 1.75; // 1.75 sigma; 1/25 of points
        double maxMax = 1 + noise * 5; // Five sigma -- very rare
        double expectMax = (minMax + maxMax) / 2;
        double delta = (maxMax - minMax) / 2;
        assertEquals(expectMax, getMaxAbsval(), delta);
    }

    @Test
    public void dipTest() {
        assertEquals(400e6, getDipFreq(), 0.01e6);
        // TODO: SDV calculation needs work
        assertEquals(0.0021e6, getDipFreqSdv(), 0.002e6);
    }

    @Test
    public void wrapAngleTest() {
        double pi = Math.PI;
        double piX2 = 2 * Math.PI;
        double piX3 = 3 * Math.PI;
        double piX4 = 4 * Math.PI;
        double eps = 1e-12;
        double dt = 0.01;
        double[] targets = {dt, pi - dt, pi + dt, piX2 - dt};
        for (double t0 : targets) {
            for (int i = -2; i <= 2; i++) {
                double t = t0 + i * piX2;
                double a = t + pi - eps;
                assertEquals(a, wrapAngle(a, t), eps);
                a = t + pi + eps;
                assertEquals(a - piX2, wrapAngle(a, t), eps);
                a = t + piX2;
                assertEquals(a - piX2, wrapAngle(a, t), eps);
                a = t + piX3 - eps;
                assertEquals(a - piX2, wrapAngle(a, t), eps);
                a = t + piX3 + eps;
                assertEquals(a - piX4, wrapAngle(a, t), eps);
            }
       }
    }

}
