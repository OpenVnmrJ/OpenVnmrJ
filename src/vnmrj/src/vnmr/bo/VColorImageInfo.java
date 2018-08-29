/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.bo;

public class VColorImageInfo {
    private int imgId;
    private int mapId;
    private int order;
    private int transparency;
    private int alpha;
    private String imageName;
    private String mapName;

    public VColorImageInfo(int id, int mapid, int transparency, String name) {
        this.imgId = id;
        this.mapId = mapid;
        this.order = 1;
        this.imageName = name;
        setTransparency(transparency);
    }

    public VColorImageInfo(int i, String name) {
         this(i, 4, 0, name);
    }

    public void setOrder(int i) {
        order = i;
    }

    public int getOrder() {
        return order;
    }

    public void setMapId(int i) {
        mapId = i;
    }

    public void setId(int i) {
        imgId = i;
    }

    public int getId() {
        return imgId;
    }

    public int getMapId() {
        return mapId;
    }

    public void setMapName(String str) {
        mapName = str;
    }

    public String getMapName() {
        return mapName;
    }

    public int getTransparency() {
        return transparency;
    }

    public void setTransparency(int v) {
       transparency = v;
       float fv = (float) v;
       if (fv > 99.0f)
           fv = 100.0f;
       else if (fv < 0.0f)
           fv = 0.0f;
       float newSet = 1.0f - fv / 100.0f;
       alpha = (int) (255.0f * newSet);
    }

    public int getAlpha() {
       return alpha;
    }

    public void setText(String str) {
        imageName = str;
    }

    public void clear() {
        imageName = "  ";
        mapId = -1;
        imgId = -1;
        order = -1;
    }

    public String toString() {
        return imageName;
    }

}

