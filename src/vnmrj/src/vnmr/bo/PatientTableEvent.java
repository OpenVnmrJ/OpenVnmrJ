/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.bo;

import java.util.EventObject;
import java.util.List;

/**
 * Title:       Patient table Event Object
 * Description:  Patient Table Events
 */

public class PatientTableEvent extends EventObject {

  static public final int TABLE_EVENT_UNKNOWN = 0;
  static public final int TABLE_EVENT_MOTION_UP = 1;
  static public final int TABLE_EVENT_MOTION_DOWN = 2;
  static public final int TABLE_EVENT_MOTION_IN = 3;
  static public final int TABLE_EVENT_MOTION_OUT = 4;
  static public final int TABLE_EVENT_QWERY_CMPLT = 5;

  private int tableEventType;
  private Object data;
  private Object eventCmd;

  public PatientTableEvent(Object source) {
     super(source);
     tableEventType = TABLE_EVENT_UNKNOWN;
  }

  public PatientTableEvent(Object source, int eventType, Object cmd) {
     super(source);
     tableEventType = eventType;
     eventCmd = cmd;
  }

  public void setData(Object d)
  {
      data = d;
  }

  public Object getData()
  {
     return data;
  }
  public List getDataList()
  {
     return (List) data;
  }
  public int getDataSize()
  {
     return ((List)data).size();
  }
  public Object getCmd()
  {
     return eventCmd;
  }

  /* patterned after AWTEvent class method which returns the type of event */
  public int getID() { return tableEventType; }
  public boolean isVerticalLocation() { return( isAxisY() ); }
  public boolean isAxisY() {
    String cmd = (String) eventCmd;
    return( (cmd.compareTo("Y\n") == 0) );
  }
  public boolean isHorizontalLocation() { return( isAxisX() ); }
  public boolean isAxisX() {
    String cmd = (String) eventCmd;
    return( (cmd.compareTo("X\n") == 0) );
  }

  //public boolean isDirectionUp() { return (tableEventType == TABLE_EVENT_MOTION_UP); }
  //public boolean isDirectionDown() { return (tableEventType == TABLE_EVENT_MOTION_DOWN); }
  //public boolean isDirectionIn() { return (tableEventType == TABLE_EVENT_MOTION_IN); }
  //public boolean isDirectionOut() { return (tableEventType == TABLE_EVENT_MOTION_OUT); }

}
