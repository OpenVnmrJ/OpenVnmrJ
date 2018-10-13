/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 * 
 *
 */
import javax.swing.SwingUtilities;

/**
 * An abstract class that you subclass to perform
 * GUI-related work in a dedicated thread.
 * For instructions on using this class, see 
 * http://java.sun.com/products/jfc/swingdoc-current/threads2.html
 */

public abstract class SwingWorker {

    private Object value;  // see getValue(), setValue()
    private Thread thread;
    private ThreadVar threadVar;

    //Compute the value to be returned by the <code>get</code> method. 

    public abstract Object construct();

    //Class to maintain reference to current worker thread
    //under separate synchronization control.

    private static class ThreadVar {
        private Thread thread;
        ThreadVar(Thread t) { thread = t; }
        synchronized Thread get() { return thread; }
        synchronized void clear() { thread = null; }
    }

    //Get the value produced by the worker thread, or null if it 
    //hasn't been constructed yet.

    protected synchronized Object getValue() { 
        return value; 
    }

    //Set the value produced by worker thread 

    private synchronized void setValue(Object x) { 
        value = x; 
    }

    //Called on the event dispatching thread (not on the worker thread)
    //after the "construct" method has returned.

    public void finished() {
    }

    //A new method that interrupts the worker thread.  Call this method
    //to force the worker to abort what it's doing.

    public void interrupt() {
        Thread t = threadVar.get();
        if (t != null) {
            t.interrupt();
        }
        threadVar.clear();
    }

    //Return the value created by the "construct" method.  
    //Returns null if either the constructing thread or
    //the current thread was interrupted before a value was produced.
    //return the value created by the <code>construct</code> method
    
    public Object get() {
        while (true) {  
            Thread t = threadVar.get();
            if (t == null) {
                return getValue();
            }
            try {
                t.join();
            }
            catch (InterruptedException e) {
                Thread.currentThread().interrupt(); // propagate
                return null;
            }
        }
    }

    //Start a thread that will call the "construct" method and then exit.

    public SwingWorker() {

        final Runnable doFinished = new Runnable() {
           public void run() { finished(); }
        };

        Runnable doConstruct = new Runnable() { 
            public void run() {
                try {
                    setValue(construct());
                }
                finally {
                    threadVar.clear();
                }

                SwingUtilities.invokeLater(doFinished);
            }
        };

        Thread t = new Thread(doConstruct);
        threadVar = new ThreadVar(t);
        t.start();
    }
}
