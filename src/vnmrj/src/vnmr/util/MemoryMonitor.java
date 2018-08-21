/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;
import java.awt.*;
import java.awt.event.*;
import java.awt.image.BufferedImage;
import java.awt.geom.Line2D;
import java.awt.geom.Rectangle2D;
import javax.swing.JFrame;
import javax.swing.JPanel;

/**
 * Tracks Memory allocated & used, displayed in graph form.
 */
public class MemoryMonitor extends JFrame {

    public MonitorComponent mc;

    public MemoryMonitor() {
        mc = new MonitorComponent();
        getContentPane().add(mc);
	setTitle("VNMRJ Memory Info");
        WindowListener l = new WindowAdapter() {
            public void windowClosing(WindowEvent e) {System.exit(0);}
            public void windowDeiconified(WindowEvent e) { mc.start(); }
            public void windowIconified(WindowEvent e) { mc.stop(); }
        };
        addWindowListener(l);
	Dimension screenDim = Toolkit.getDefaultToolkit().getScreenSize();
        pack();
        setSize(new Dimension(320,120));
	setLocation(screenDim.width - 320, screenDim.height - 120);
        
    }


    public class MonitorComponent extends JPanel implements Runnable
    {
        public Thread thread;
        private int w, h;
        private BufferedImage bimg;
        private Graphics2D big;
        private Font font = new Font("Times New Roman", Font.PLAIN, 11);
        private Runtime r = Runtime.getRuntime();
        private int columnInc;
        private float usedPts[];
        private float totalPts[];
        private int pts[];
        private int pts2[];
        private int ptNum = 0;
        private int stPtr = 0;
        private int endPtr = 0;
        private int dataLen = 0;
        private int winH = 0;
        private int ascent, descent;
        private float freeMem, totalMem;
        private float maxMem = 0;
        private float minMem = 90000000;
        private float usedMem = 0;
        private Rectangle graphOutlineRect = new Rectangle();
        private Rectangle2D mfRect = new Rectangle2D.Float();
        private Rectangle2D muRect = new Rectangle2D.Float();
        private Line2D graphLine = new Line2D.Float();
        private Color graphColor = new Color(46, 139, 87);
        private Color mfColor = new Color(0, 100, 0);
        FontMetrics fm;


        public MonitorComponent() {
            setBackground(Color.black);
            addMouseListener(new MouseAdapter() {
                public void mouseClicked(MouseEvent e) {
                    if (thread == null) start(); else stop();
                }
            });
            start();
        }

        public Dimension getMinimumSize() {
            return getPreferredSize();
        }

        public Dimension getMaximumSize() {
            return getPreferredSize();
        }

        public Dimension getPreferredSize() {
            return new Dimension(140,80);
        }

            
        public void paint(Graphics g) {

            if (big == null) {
                return;
            }

	    boolean  toAdj;

            big.setBackground(getBackground());
            big.clearRect(0,0,w,h);

            freeMem = (float) r.freeMemory();
            totalMem = (float) r.totalMemory();

	    toAdj = false;
	    if (totalMem > maxMem) {
		maxMem = totalMem;
		toAdj = true;
	    }
	    if ((totalMem > 1) && (totalMem < minMem)) {
		minMem = totalMem;
	    }

            // .. Draw allocated and used strings ..
            big.setColor(Color.green);
	    int iv = (int) ((maxMem/1024000.0) * 10000.0);
	    float fv = (float) (iv / 10000.0);
	    iv = (int) ((minMem/1024000.0) * 10000.0);
	    float fv2 = (float) (iv / 10000.0);
	    int  cx = 4;
	    if (fm != null)
		cx = fm.charWidth('M') * 6;
            big.drawString("MB allocated. Max "+fv+" Min "+fv2,  cx, ascent);
	    iv = (int) ((totalMem/1024000.0) * 10000.0);
	    fv = (float) (iv / 10000.0);
            big.drawString(String.valueOf(fv),  4, ascent);
            big.drawString("MB used", cx, h-descent);
	    iv = (int)(((totalMem - freeMem)/1024000.0) * 10000.0);
	    usedMem = (float) (iv / 10000.0);
            big.drawString(String.valueOf(usedMem), 4, h-descent);

            // Calculate remaining size
            float ssH = ascent + descent;
            float remainingHeight = (float) (h - (ssH*2) - 0.5f);
            float blockHeight = remainingHeight/10;
            float blockWidth = 20.0f;
            float remainingWidth = (float) (w - blockWidth - 10);

            // .. Memory Free ..
            big.setColor(mfColor);
            int MemUsage = (int) ((freeMem / maxMem) * 10);
            int i = 0;
            for ( ; i < MemUsage ; i++) { 
                mfRect.setRect(5,(float) ssH+i*blockHeight,
                                blockWidth,(float) blockHeight-1);
                big.fill(mfRect);
            }

            // .. Memory Used ..
            big.setColor(Color.green);
            for ( ; i < 10; i++)  {
                muRect.setRect(5,(float) ssH+i*blockHeight,
                                blockWidth,(float) blockHeight-1);
                big.fill(muRect);
            }

            // .. Draw History Graph ..
            big.setColor(graphColor);
            int graphX = 30;
            int graphY = (int) ssH;
            int graphW = w - graphX - 5;
            int graphH = (int) remainingHeight;
            graphOutlineRect.setRect(graphX, graphY, graphW, graphH);
            big.draw(graphOutlineRect);

            int graphRow = graphH/10;

            // .. Draw row ..
            for (int j = graphY; j <= graphH+graphY; j += graphRow) {
                graphLine.setLine(graphX,j,graphX+graphW,j);
                big.draw(graphLine);
            }
        
            // .. Draw animated column movement ..
            int graphColumn = graphW/15;

            if (columnInc == 0) {
                columnInc = graphColumn;
            }

            for (int j = graphX+columnInc; j < graphW+graphX; j+=graphColumn) {
                graphLine.setLine(j,graphY,j,graphY+graphH);
                big.draw(graphLine);
            }

            --columnInc;

            if (pts == null) {
	        Dimension scrDim = Toolkit.getDefaultToolkit().getScreenSize();
		dataLen = scrDim.width;
                pts = new int[dataLen];
                pts2 = new int[dataLen];
                usedPts = new float[dataLen];
                totalPts = new float[dataLen];
		stPtr = 0;
		endPtr = 0;
                ptNum = 0;
            }
	    else if (dataLen < graphW) {
                int  tmp[] = new int[graphW];
                System.arraycopy(pts, 0, tmp, 0, dataLen);
                pts = new int[graphW];
                System.arraycopy(tmp, 0, pts, 0, dataLen);

                System.arraycopy(pts2, 0, tmp, 0, dataLen);
                pts2 = new int[graphW];
                System.arraycopy(tmp, 0, pts2, 0, dataLen);
		float tmpf[] = new float[graphW];
                System.arraycopy(usedPts, 0, tmpf, 0, dataLen);
                usedPts = new float[graphW];
                System.arraycopy(tmpf, 0, usedPts, 0, dataLen);
                System.arraycopy(totalPts, 0, tmpf, 0, dataLen);
                totalPts = new float[graphW];
                System.arraycopy(tmpf, 0, totalPts, 0, dataLen);
		dataLen = graphW;
		tmp = null;
		tmpf = null;
	     }
	     if (ptNum < dataLen)
		ptNum++;
	     usedPts[endPtr] = totalMem - freeMem;
	     totalPts[endPtr] = totalMem;
             pts[endPtr] = (int)(graphY+graphH*(maxMem-usedPts[endPtr])/maxMem);
             pts2[endPtr] = (int)(graphY+graphH*(maxMem-totalPts[endPtr])/maxMem);
	     endPtr++;
	     if (ptNum < 2)
		return;
	     if (endPtr >= dataLen)
		endPtr = 0;
	     if (ptNum >= dataLen) {
		stPtr = dataLen + endPtr - graphW - 1;
	     }
	     else
		stPtr = endPtr - graphW - 1;
	     if (stPtr >= dataLen)
		stPtr = stPtr - dataLen - 1;
	     if (stPtr < 0)
		stPtr = 0;
	     
	     if (toAdj || (winH != graphH)) {
		winH = graphH;
		for (int m = 0; m < ptNum; m++) { 
		    pts[m] = (int)(graphY+graphH*(maxMem-usedPts[m])/maxMem);
		    pts2[m] = (int)(graphY+graphH*(maxMem-totalPts[m])/maxMem);
		}
	     }
	     int j = graphW - ptNum + 1;
	     if (j < 0)
		j = 0;

             big.setColor(Color.yellow);
	     int s = stPtr;
	     int s2 = stPtr+1;
	     int gx = graphX + j;
             for (int k=j; k < graphW; k++) {
		    if (s2 >= dataLen)
			s2 = 0;
                    if (pts[s] != pts[s2]) {
                       big.drawLine(gx, pts[s], gx, pts[s2]);
                    } else {
                       big.fillRect(gx, pts[s], 1, 1);
		    }
		    gx++;
		    s = s2;
		    s2++;
	     }
             big.setColor(Color.red);
	     s = stPtr;
	     s2 = stPtr+1;
	     gx = graphX + j;
             for (int k=j; k < graphW; k++) {
		    if (s2 >= dataLen)
			s2 = 0;
                    if (pts2[s] != pts2[s2]) {
                       big.drawLine(gx, pts2[s], gx, pts2[s2]);
                    } else {
                       big.fillRect(gx, pts2[s], 1, 1);
		    }
		    gx++;
		    s = s2;
		    s2++;
	     }
            g.drawImage(bimg, 0, 0, this);
        }


        public void start() {
            thread = new Thread(this);
            thread.setPriority(Thread.MIN_PRIORITY);
            thread.setName("MemoryMonitor");
            thread.start();
        }


        public synchronized void stop() {
            thread = null;
            notify();
        }


        public void run() {

            Thread me = Thread.currentThread();

            while (thread == me && !isShowing() || getSize().width == 0) {
                try {
                    Thread.sleep(500);
                } catch (InterruptedException e) { thread = null; return; }
            }

            while (thread == me && isShowing()) {
                Dimension d = getSize();
                if (d.width != w || d.height != h) {
                    w = d.width;
                    h = d.height;
                    bimg = (BufferedImage) createImage(w, h);
                    big = bimg.createGraphics();
                    big.setFont(font);
                    fm = big.getFontMetrics(font);
                    ascent = (int) fm.getAscent();
                    descent = (int) fm.getDescent();
                }
                repaint();
                try {
                    thread.sleep(999);
                } catch (InterruptedException e) { break; }
            }
            thread = null;
        }
    }
}
