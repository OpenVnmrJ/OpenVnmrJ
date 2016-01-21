/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.probeid;

import java.util.ArrayDeque;
import java.util.Deque;

/**
 * TODO: this should really be a bunch of small classes represented
 *       through a ProbeIdCmd interface.
 *       
 * @author dirk
 *
 */
public class ProbeIdCmd {
    public  Integer m_notif   = null;    // process id to signal startup completion
	private Integer m_timeout = null;
	private String  m_cmd     = null;
	private String  m_key     = null;
	private String  m_default = null;
	private String  m_path    = null;
	private String  m_cfg     = null;
	private String  m_tree    = "global";
	private String  m_id      = null;
	private String  m_host    = null;
	private String  m_cache   = null;
	private String  m_mnt     = null;
	private Boolean m_sys     = null;    // user specified system file
	private Boolean m_usr     = null;    // user specified user file
	public  boolean reattach  = false;
	public  String  cache     = null;
	public  Boolean crypto    = null;
	Deque<String>   m_param   = new ArrayDeque<String>();
	Deque<String>   m_opts    = new ArrayDeque<String>(); // generic options

	// setters
	public boolean moreOpts()            { return !m_opts.isEmpty(); }
	public ProbeIdCmd tree(String tree)  { m_tree  = tree; return this; }
	public ProbeIdCmd cfg(String cfg)    { m_cfg   = cfg; return this; }
	public ProbeIdCmd key(String key)    { m_key   = key; return this; }
	public ProbeIdCmd path(String path)  { m_path  = path; return this; }
	public ProbeIdCmd param(String par)  { m_param.addLast(par); return this; }
	public ProbeIdCmd opt(String value)  { m_opts.addLast(value); return this; }
	public ProbeIdCmd sys(boolean value) { m_sys = value; return this; }
	public ProbeIdCmd usr(boolean value) { m_usr = value; return this; }
	public ProbeIdCmd id(String value)   { m_id = value; return this; }
	public ProbeIdCmd host(String value) { m_host = value; return this; }
	public ProbeIdCmd notif(String id)   { m_notif = Integer.valueOf(id); return this;}
	public ProbeIdCmd timeout(String ms) { m_timeout = Integer.valueOf(ms); return this; }
	public ProbeIdCmd timeout(int ms)    { m_timeout = ms; return this; }
	// getters
	public String nextOpt()  { return m_opts.pollFirst(); }
	public String key()      { return m_key; }
	public String cmd()      { return m_cmd != null ? m_cmd : m_default; }
	public String path()     { return m_path; }
	public String cfg()      { return m_cfg; }
	public String param()    { return m_param.peekFirst(); }
	public String nextParam(){ return m_param.pollFirst(); }
	public String tree()     { return m_tree; }
	public String id()       { return m_id; }
	public String host()     { return m_host; }
	public String cache()    { return m_cache; }
	public String mnt()      { return m_mnt; }
	public Boolean sys()     { return m_sys; }
	public boolean sysOnly() { return sys() != null && sys(); }
	public Boolean usr()     { return m_usr; }
	public Integer notif()   { return m_notif; }
	public Integer timeout() { return m_timeout; }
	
	public String[] getOpts() 
	{ return (String[]) m_opts.toArray(new String[m_opts.size()]); }

	public void set(String value, String key) throws Exception {
		m_key = key;
		set(value);
	}
	public String set(String value) throws Exception {
		if (m_cmd != null) 
			throw new Exception("Only one command option allowed per call");
		return m_cmd = value;
	}
	boolean is(String cmd)   { return cmd().equals(cmd); }
	ProbeIdCmd(String dflt)  { m_default = dflt; }
	ProbeIdCmd(String dflt, String opt) { this(dflt); opt(opt); }
	
	// caller specifies an ID only for off-line processing
	public boolean offline() { return id() != null || cmd().equals(ProbeId.CMD_START); }

	// qualify command syntax for options with values, i.e. -arg foo=bar
	public boolean is(String cmd, String[] args, int i) {
		return (i+1<args.length && args[i].equals(cmd));
	}
	
	// set system property
	public void property(String assignment) {
		try {
			if (assignment.contains("=")) {
				String pair[] = assignment.split("=");
				if (pair.length==2)
					System.setProperty(pair[0], pair[1]);
			} else {
				ProbeIdIO.error("invalid Probe ID server property assignment \""+assignment+"\"");
			}
		} catch (Exception e) {
			ProbeIdIO.error(e.getMessage());
		}
	}
}
