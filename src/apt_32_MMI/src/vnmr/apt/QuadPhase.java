/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.apt;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;

/**
 * This class holds information about a previously determined Quad Phase.
 */
public class QuadPhase {
    public static final int MAX_QUALITY = 10;

    public static final int MIN_GOOD_QUALITY = 5;

    public static final int MAX_SAVED_QUALITY = 4;

    /**
     * The date of the determination.
     */
    private long date = System.currentTimeMillis();

    /**
     * The phase determined.
     */
    private int phase = 0;

    /**
     * The reliability of the determination.
     * From 0 (completely uncertain) to MAX_QUALITY (certain).
     */
    private int quality = MAX_QUALITY;

    /**
     * This default constructor gives an instance with phase=0 and the highest
     * quality -- appropriate for when the calibration is first made.
     */
    public QuadPhase() {
    }

    public QuadPhase(int phase, int quality) {
        this.phase = phase;
        this.quality = quality;
    }

    /**
     * @return the date
     */
    public long getDate() {
        return date;
    }

    /**
     * @param date the date to set
     */
    public void setDate(long date) {
        this.date = date;
    }

    /**
     * @return the quadPhase
     */
    public int getPhase() {
        return phase;
    }

    /**
     * @param quadPhase the quadPhase to set
     */
    public void setPhase(int quadPhase) {
        this.phase = quadPhase;
    }

    /**
     * @return the quality
     */
    public int getQuality() {
        return quality;
    }

    /**
     * @param quality the quality to set
     */
    public void setQuality(int quality) {
        this.quality = quality;
    }

    /**
     * @param calPath The path to a calibration file.
     * @return The path to the QuadPhase file.
     */
    private static String getPhasePathFromCalPath(String calPath) {
        File calFile = new File(calPath);
        String name = calFile.getName().replaceAll("\\.zip$","");
        String path = calFile.getParent() + "/QuadPhases/" + name;
        return path;
    }

    public static QuadPhase readPhase(String calPath) {
        QuadPhase savedPhase = new QuadPhase();
        String path = getPhasePathFromCalPath(calPath);
        BufferedReader in = TuneUtilities.getReader(path);
        if (in != null) {
            try {
                String line;
                while ((line = in.readLine()) != null) {
                    String[] toks = line.split(" +", 2);
                    if (toks.length == 2) {
                        String key = toks[0];
                        String val = toks[1];
                        if ("phase".equals(key)) {
                            savedPhase.phase = Integer.parseInt(val);
                        } else if ("quality".equals(key)) {
                            savedPhase.quality = Integer.parseInt(val);
                        } else if ("date".equals(key)) {
                            savedPhase.date = Long.parseLong(val);
                        }
                    }
                }
            } catch (IOException ioe) {
            } finally {
                try {
                    in.close();
                } catch (Exception e) {}
            }
        }
        return savedPhase;
    }

    public static void writePhase(String calPath, QuadPhase phase) {
        String path = getPhasePathFromCalPath(calPath);
        File dir = new File(path).getParentFile();
        dir.mkdirs();
        dir.setWritable(true, false); // Directory writable by all
        String contents = "phase " + phase.phase + "\n";
        contents += "quality " + phase.quality + "\n";
        contents += "date " + phase.date + "\n";
        TuneUtilities.writeFile(path, contents);
        new File(path).setWritable(true, false); // Set writable by all
    }

    public String toString() {
        return "phase=" + phase + ",quality=" + quality;
    }

    public boolean isGoodQuality() {
        return quality >= MIN_GOOD_QUALITY;
    }
}
