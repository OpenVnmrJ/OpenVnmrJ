/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.probeid;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.FileReader;
import java.io.IOException;
import java.io.File;
import java.util.Collection;
import java.util.ListIterator;
import java.util.Vector;

public class ProbeIdKeyValueIO implements ProbeId {
	/**
	 * KeyValuePair class holds a key value String pair.
	 * It is initialized with a line read from a file,
	 * from which the key and value Strings are extracted.
	 * 
	 * Values can be read and written simultaneously - the
	 * read values are always the original value, until the
	 * writer is flushed.  Geared towards bulk updates.
	 * 
	 * @author dirk
	 *
	 */
	private KeyValueCache m_cache = null;
	
	static class KeyValuePair {
		public String key;
		public String value;
		KeyValuePair(String[] pair) {
			assert(pair.length==2);
			key = pair[0];
			value = pair[1];
		}
		// use a factory method
		static KeyValuePair get(String line) {
			ProbeIdDb.Tokenizer tok = new ProbeIdDb.Tokenizer(line);
			String key = tok.hasMoreTokens() ? tok.nextToken().trim() : null;
			String value = tok.hasMoreTokens() ? tok.nextToken().trim() : null;
			if (key != null && value != null) 
				return new KeyValuePair(key, value);
			return null;
		}
		
		public KeyValuePair(String key, String value) {
			this.key = key; 
			this.value = value;
		}
	}
	
	static class Group {
		public String id   = null;	// "Probe:", "H1:", etc.
		public String header = null;     // typically "Parameters"
		public Vector<KeyValuePair> parameters = new Vector<KeyValuePair>();
		public Group(ListIterator<KeyValuePair> iterator) {
			while (iterator.hasNext()) {
				KeyValuePair pair = iterator.next();
				if (pair != null && pair.key.endsWith(KEY_VALUE_NUCLEUS_DELIM)) {
					if (id == null) {
						id = pair.key;
						header = pair.value;
					} else { // end of nucleus definition
						iterator.previous(); // don't get ahead of ourselves
						return;
					}
				} else if (id != null && pair != null) {
					// we read the header already
					parameters.add(pair);
				}
			}
		}
	}
	
	static class Table {
		public Vector<Group> groups = new Vector<Group>();
		Table(Vector<KeyValuePair> pairs) {
			ListIterator<KeyValuePair> i = pairs.listIterator();
			while (i.hasNext()) {
				Group group = new Group(i);
				if (group != null && !group.id.equals("NAME:"))
					groups.add(group);
			}
		}
	}

	public Table getTable() {
		Vector<KeyValuePair> pairs = new Vector<KeyValuePair>();
		ListIterator<String> line = m_cache.getIterator();
		while (line.hasNext())
			pairs.add(KeyValuePair.get(line.next()));
		return new Table(pairs);
	}
	
	public static void write(BufferedWriter out, String key, String value) 
		throws IOException
	{
		String line = String.format(KEY_VALUE_FORMAT, key, value);
		out.write(line+'\n');
	}
	
	class KeyValueCache {
		private Vector<String> m_contents = null;
		ListIterator<String>   m_iterator = null;
		
		void reset() {
			if (m_contents != null)
				m_iterator = m_contents.listIterator();
			else
				m_iterator = null;
		}
		
		ListIterator<String> getIterator() {
			return m_contents.listIterator();
		}
		
		String setIfMatch(String line, KeyValuePair replace) 
		{
			KeyValuePair current = KeyValuePair.get(line);
			if (current != null) {
				ProbeIdIO.debug("checking <"+current.key+","+current.value+">");
				if (current.key.equals(replace.key)) {
					String oldValue = line;//current.value;
					line = String.format(KEY_VALUE_FORMAT, replace.key, replace.value);
					m_iterator.set(line);
					return oldValue;
				}
			}
			return null;
		}
		
		String set(KeyValuePair replacement) {
			ListIterator<String> start = m_iterator;
			// start search from the last place we left off
			while (m_iterator.hasNext()) {
				String line = m_iterator.next();
				String replaced = setIfMatch(line, replacement);
				if (replaced != null)
					return replaced;
			}
			// wrap search around to the beginning
			m_iterator = m_contents.listIterator(0);
			while (m_iterator.nextIndex() != start.nextIndex()) {
				String replaced = setIfMatch(m_iterator.next(), replacement);
				if (replaced != null)
					return replaced;
			}
			return null;
		}
		
		void commit(BufferedWriter writer) throws IOException {
			ProbeIdIO.info("saving "+Integer.toString(m_contents.size())+" entries");
			for (String line : m_contents)
				writer.write(line+'\n');
			writer.close();
		}
		
		KeyValueCache(BufferedReader reader) throws IOException {
			m_contents = new Vector<String>();
			String line;
			while ((line = reader.readLine()) != null)
				m_contents.add(line);
			m_iterator = m_contents.listIterator(0);			
		}
	}
	
	public Collection<KeyValuePair> getKeyValue(String paramList) {
		Vector<KeyValuePair> result = new Vector<KeyValuePair>();
		String[] pairs = paramList.split(KEY_VALUE_LIST_SEPARATOR);
		for (String pair : pairs) {
			String[] param = pair.split(KEY_VALUE_PAIR_SEPARATOR);
			KeyValuePair kv = new KeyValuePair(param);
			result.add(kv);
		}
		return result;
	}
	
	public Collection<String> setParams(String pairList) throws IOException {
		Collection<String> results = new Vector<String>();
		Collection<KeyValuePair> params = getKeyValue(pairList);
		for (KeyValuePair param : params) {
			String oldValue = m_cache.set(param);
			if (oldValue == null) {
				results = null;  // it's all or nothing
				break;
			} else results.add(oldValue);
		}
		return results;
	}
	 
	public void commit(BufferedWriter writer) throws IOException {
		m_cache.commit(writer);
	}
	
	ProbeIdKeyValueIO(BufferedReader reader) throws IOException {
		m_cache = new KeyValueCache(reader);
	}

	ProbeIdKeyValueIO(File file) throws IOException {
		this(new BufferedReader(new FileReader(file)));
	}
}
