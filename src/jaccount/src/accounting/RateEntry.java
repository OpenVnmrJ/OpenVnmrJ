/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package accounting;

public class RateEntry {
    String day;
    String time;
    String rate;
    String login;
    String loginhr;
    String go;
    String gohr;
    int    dayOfWeek;
    Double nMinutes;

    public RateEntry() {
    }

    public void day(String day)        {this.day = day;}
    public String day()                {return this.day;}

    public void time(String time)      {this.time = time;}
    public String time()               {return this.time;}

    public void login(String login)    {this.login = login;}
    public String login()              {return this.login;}
    
    public void loginhr(String loginhr){this.loginhr = loginhr;}
    public String loginhr()            {return this.loginhr;}
    
    public void go(String go)          {this.go = go;}
    public String go()                 {return this.go;}
    
    public void gohr(String gohr)      {this.gohr = gohr;}
    public String gohr()               {return this.gohr;}
    
    public void rate(String rate)      {this.rate = rate;}
    public String rate()               {return this.rate;}
    
    public void nMinutes(Double nMinutes) {this.nMinutes = nMinutes;}
    public Double nMinutes()           {return this.nMinutes;}
    
    public void dayOfWeek(int nDay)      {this.dayOfWeek = nDay;}
    public int dayOfWeek()               {return this.dayOfWeek;}
}
