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
import java.util.*;
// import java.awt.peer.*;
import java.awt.event.*;
import java.awt.datatransfer.Clipboard;
import java.awt.datatransfer.DataFlavor;
import java.awt.datatransfer.StringSelection;
import java.awt.datatransfer.Transferable;
import java.awt.dnd.*;
import java.beans.PropertyChangeEvent;
import java.beans.PropertyChangeListener;
import java.io.*;

import javax.swing.*;
import javax.swing.Timer;

import vnmr.bo.*;
import vnmr.ui.*;
import vnmr.templates.*;


/**
 * Class has functions (i.e., static methods) that serve as
 * general utilities.
 *
 */
public final class ParamEditUtil implements VObjDef,EditListenerIF {
    public final static int leftUp = 1;
    public final static int leftDown = 2;
    public final static int rightUp = 3;
    public final static int rightDown = 4;
    public final static int MGAP = 8;
    private final static int minSize = 8;
    private static JComponent editObj = null;
    private static JComponent editObj2 = null; // for VTextWin
    private static VObjIF editVobj = null;
    private static VObjIF editVobj2 = null;
    private static MouseAdapter ml = null;
    private static MouseMotionAdapter mvl = null;
    private static int  x, y, w, h, mw, mh;
    private static int  x2 = -999;
    private static int  y2;
    private static int  orgX, orgY;
    private static int  rx, ry;
    private static int  cursor = 0;
    private static int  difX, difY;
    private static int  difAX, difAY;
    private static int  snapGap = 5;
    private static int  maxH = 100;
    private static int  maxW = 100;
    private static int  panX, panY, panX2, panY2;
    private static boolean  snapFlag = true;
    private static boolean  isDraging = false;
    private static boolean  isEditing = false;
    private static boolean  inTrashCan = false;
    private static boolean  inNewPanel = false;
    private static boolean  isOutside = false;
    private static boolean  outOfBound = false;
    private static boolean  getGraphics = false;
    protected static boolean bLayeredGroup = false;
    private static ParamPanel editPanel;
    private static ParamPanel pastePanel=null;
    private static Point pastePoint=new Point(0,0);
    private static boolean newObject = false;
    private static boolean autoscroll = false;

    private static Graphics g;
/*
    private static DragGestureListener dragListener;
*/
    private static KeyAdapter kl = null;
    private static boolean lastWasPressed = false;
    private static FocusListener fl = null;
    private static boolean  isResizing = false;
    private static boolean  isCopying = false;
    private static JScrollPane scrlPanel;
    private static JViewport viewPort;
    private static Point     viewPoint;
    private static Rectangle scrlBound;
    private static Rectangle panelBound;
    private static VPseudo   dummy = new VPseudo();
    private static VPseudo2 dummy2 = new VPseudo2();
    private static AppIF    appIf;
    private static MouseEvent  repeatEvent;
    private static Timer       timer = null;
    private static Cursor  moveCursor = DragSource.DefaultMoveDrop;
    private static Cursor  outCursor = DragSource.DefaultMoveNoDrop;
    private static Cursor  copyCursor = DragSource.DefaultCopyDrop;
    private static Cursor  defCursor = null;
    private static JFrame frame = null;
    private static Component glassPan = null;
    private static Component cursorObj = null;
//    private static ComponentPeer peer;
    private final static int  MOVE = 21;
    private final static int  OUT = 22;
    private final static int  COPY = 23;
    private static boolean clipempty=true;
    private static File tmpFile=null;
    protected static JMenuItem cut = null;
    protected static JMenuItem copy = null;
    protected static JMenuItem paste = null;
    protected static JMenuItem clear = null;
    private static EMenu menu=null;//new EMenu();
    private static HashMap<KeyStroke, Action> actionMap = null;
    private static Clipboard clipboard=null;

    private static void setDebug(String s){
        if(DebugOutput.isSetFor("ParamEditUtil"))
            Messages.postDebug("ParamEditUtil : "+s);
    }
    /**
     * Constructor is private, so nobody can construct an instance.
     */
    public ParamEditUtil() { 
    	setupKeyListener();
    	setAdapters();
    	ParamInfo.addEditListener(this);
    }

	/**
	 * Set up global key listeners for edit menu operations
	 */
	private static void setupKeyListener() {
		clipboard=Toolkit.getDefaultToolkit().getSystemClipboard();
		actionMap = new HashMap<KeyStroke, Action>();
		KeyStroke key = KeyStroke.getKeyStroke(KeyEvent.VK_C, KeyEvent.CTRL_DOWN_MASK);
		actionMap.put(key, new AbstractAction("copy") {
			@Override
			public void actionPerformed(ActionEvent e) {
				if(isEditing && e.getSource()==dummy2){
			        //System.out.println("global: copyobj");
					copyObject();	
				}
			}
		});
		key = KeyStroke.getKeyStroke(KeyEvent.VK_V, KeyEvent.CTRL_DOWN_MASK);
		actionMap.put(key, new AbstractAction("paste") {
			@Override
			public void actionPerformed(ActionEvent e) {
				if(isEditing /*&& pastePanel !=null*/){
					Transferable content=clipboard.getContents(clipboard);
					if(content !=null){
						try {
							if (content.isDataFlavorSupported(DataFlavor.stringFlavor)) {
								String data=(String)content.getTransferData(DataFlavor.stringFlavor);
								if(tmpFile !=null && data.equals(tmpFile.getName()))
									pasteObject();
							}
							return;
						}
						catch(Exception ex){} // just means some other type of data is in the clipboard					
					}
				}
			}
		});
	
		key = KeyStroke.getKeyStroke(KeyEvent.VK_X, KeyEvent.CTRL_DOWN_MASK);
		actionMap.put(key, new AbstractAction("cut") {
			@Override
			public void actionPerformed(ActionEvent e) {
				if(isEditing && e.getSource()==dummy2)
					cutObject();
			}
		});
		key = KeyStroke.getKeyStroke(KeyEvent.VK_BACK_SPACE,0);
		actionMap.put(key, new AbstractAction("clear") {
			@Override
			public void actionPerformed(ActionEvent e) {
				if(isEditing && e.getSource()==dummy2)
					clearObject();
			}
		});

		KeyboardFocusManager kfm = KeyboardFocusManager
				.getCurrentKeyboardFocusManager();
		kfm.addKeyEventDispatcher(new KeyEventDispatcher() {

			@Override
			public boolean dispatchKeyEvent(KeyEvent e) {
				KeyStroke keyStroke = KeyStroke.getKeyStrokeForEvent(e);
				if (actionMap.containsKey(keyStroke)) {
					final Action a = actionMap.get(keyStroke);
					final ActionEvent ae = new ActionEvent(e.getSource(), e
							.getID(), null);
					SwingUtilities.invokeLater(new Runnable() {
						@Override
						public void run() {
							a.actionPerformed(ae);
						}
					});
					// retuning false allows other listeners (e.g. text editors) to
					// process cntrl keys
					return false;
				}
				return false;
			}
		});
	}

    public static void clickInPanel(MouseEvent  e) {
    	Component c=e.getComponent();
    	pastePoint=e.getPoint();
    	testNewPanel((JComponent)c);
		// rt-mouse-click inside selected object (copy,cut,clear)
		//   - on selection, dummy2 is set from editObj and assigned mouse listener
    	if(dummy2 == c || dummy==c){
			cut.setEnabled(true);
			copy.setEnabled(true);
			clear.setEnabled(true);
    	}
    	else{
			ParamPanel layout=getEditPanel((JComponent)c); // find panel parent
			if(layout==editPanel){
				pastePanel=editPanel;
			}
			else {
				pastePanel=layout;
				pastePanel.requestFocus();
			}
			cut.setEnabled(false);
			copy.setEnabled(false);
			clear.setEnabled(false);
			if(clipempty)
				paste.setEnabled(false);
			else
				paste.setEnabled(true);			
    	}
    }
    
    /** show popup menu (rt-mouse click)*/
    public static void menuAction(MouseEvent  e) {
		Component c=e.getComponent();
    	Point p=e.getPoint();
    	menu.show(c,(int)p.getX(),(int)p.getY());
    }
    public static void cutObject(){
		copyVobj();
		removeVobj();
    }
    public static void copyObject(){
		copyVobj();
    }
    public static void clearObject(){
		removeVobj();
    }

    public static void setAutoScroll(boolean b){
		autoscroll=b;
    }
    public static boolean getAutoScroll(){
		return autoscroll;
    }

    public static void testNewPanel(JComponent p){
    	ParamPanel panel=getEditPanel(p);
    	if(panel !=null && panel != pastePanel){
    		pastePanel=panel;
    		setDebug("set ParamPanel:"+panel.getPanelName());
    		//cursorObj = pastePanel.getTopLevelAncestor();
    		inNewPanel=true;
    	}
    }

    public static void pasteObject(){
        if(editVobj instanceof VTabbedPane){
            String f = tmpFile.getAbsolutePath();
            ((VTabbedPane)editVobj).pasteXmlObj(f);
            return;
        }
    	if(pastePanel !=editPanel){
            setEditPanel(pastePanel);
            ParamInfo.setEditPanel(pastePanel,true);
    	}
		pasteVobj(pastePanel,pastePoint);
    }

   public static void setChangedPage(boolean b){
        if(b && newObject && editPanel!=null && editVobj!=null){
            newObject=false;
        }
        ParamInfo.setChangedPage(b);
    }

    public static void resetCursor() {
        dummy2.setCursor(null);
        dummy.setCursor(null);
        if (cursorObj != null)
           cursorObj.setCursor(null);
        cursor = 0;
    }

	public static void removeMouseListener(JComponent comp) {
		if (comp instanceof VObj) {
			((VObj) comp).removeMouseListeners(ml, mvl);
		} else {
			comp.removeMouseListener(ml);
			comp.removeMouseMotionListener(mvl);
		}
		if (comp instanceof VGroupIF)
			cleanListeners(comp);
	}
	public static void addMouseListener(JComponent comp) {
		if (comp instanceof VObj) {
			((VObj) comp).addMouseListeners(ml, mvl);
		} else {
			comp.addMouseListener(ml);
			comp.addMouseMotionListener(mvl);
		}
		if (comp instanceof VGroupIF)
			addListeners(comp);
	}
    public static void clearDummyObj() {
        if (editPanel != null) {
            editPanel.remove(dummy2);
            editPanel.remove(dummy);
        }
    }

    public static boolean isEditObj(VObjIF obj){
    	return (editVobj==null || obj==null || obj!=editVobj)?false:true;
    }

    public static VObjIF getEditObj(){
    	return editVobj;
    }
    public static ParamPanel getLayout(){
    	return editPanel;
    }
    
    private static void setViewport(Container  pp){
	    scrlPanel = null;
	    viewPort = null;
	    while (pp != null) {
	       if (pp instanceof JScrollPane) {
	            scrlPanel = (JScrollPane) pp;
	            viewPort = scrlPanel.getViewport();
	            scrlBound = scrlPanel.getBounds();
	            break;
	       }
	       pp = pp.getParent();
	    }
    }
    private static void setAdapters(){
        if(menu==null)
            menu=new EMenu();
        if (ml == null) {
            ml = new MouseAdapter() {
               public void mousePressed(MouseEvent evt) {
                   if (getGraphics) {
                        g = editPanel.getGraphics();
                        getGraphics = false;
                   }
                   mPress(evt);
               }

               public void mouseReleased(MouseEvent evt) {
                    boolean  brelocateDummy = false;
                    if (isDraging || isResizing || isCopying)
                        brelocateDummy = true;
                    else
                        brelocateDummy = false;
                    mRelease(evt);
                    if (glassPan != null) {
                        glassPan.setVisible(false);
                    }
                    if (brelocateDummy)
                        relocateDummy();
                    if (editPanel != null)
                        editPanel.requestFocus();
               }
               public void mouseExited(MouseEvent evt) {
                    if (!isDraging && !isResizing ) {
                       mRelease(evt);
                    }
               }

               public void mouseClicked(MouseEvent evt) {
                    int clicks = evt.getClickCount();
                    int modifier = evt.getModifiers();
                    clickInPanel(evt);

                    if((modifier & InputEvent.BUTTON3_MASK) !=0){
                        menuAction(evt);
                    }
                    else if (clicks >= 2) {
                        if ((modifier & (1 << 4)) != 0) {
                            selectNextVobj(evt);
                        }
                    }
               }

           };
           dummy2.addMouseListener(ml);
        }
        if (mvl == null) {
            mvl = new MouseMotionAdapter() {
                  public void mouseDragged(MouseEvent evt) {
                          mDrag(evt);
                  }
                  public void mouseMoved(MouseEvent evt) {
                      if (!bLayeredGroup)
                          mMove(evt);
                  }
            };
            dummy2.addMouseMotionListener(mvl);
        }
        if (kl == null) {
            kl = new KeyAdapter() {
            	// Note: There appears to be a long standing bug for java apps running
            	//       in Linux where multiple keyPressed and keyReleased calls can get
            	//       generated for each key event (single or repeated).
            	//       When this happens in the panel editor it looks like the grid spacing
            	//       has suddenly doubled (or tripled) from the specified gap value.
            	//       A workaround is to only respond to the first keyReleased 
            	//       event that follows a (possibly replicated) keyPressed event
                public void keyTyped(KeyEvent e) {
                    e.consume();
                }
                public void keyPressed(KeyEvent e) {
                    e.consume();
                    lastWasPressed=true;
                }
                public void keyReleased(KeyEvent e) {
                    e.consume();
                    if(lastWasPressed){
	                    int k = e.getKeyCode();
	                    int m = e.getModifiers();
	                    if (m == 0){
	                        if(k==java.awt.event.KeyEvent.VK_BACK_SPACE)
	                            clearObject();
	                        else{
	                            keyMove(k);
	                            relocateDummy();
	                        }
	                    }
	                    else if (m == Event.CTRL_MASK){
	                        switch(k){
	                        case java.awt.event.KeyEvent.VK_C:
	                            copyObject();
	                            break;
                            case java.awt.event.KeyEvent.VK_X:
                                cutObject();
                                break;
                            case java.awt.event.KeyEvent.VK_V:
                                pasteObject();
                                break;
                            default:
                                keyResize(k);
                                relocateDummy();                                    
	                        }
	                    }
                    }
                    lastWasPressed=false;
                }
            };
        }

        if (fl == null) {
            fl = new FocusAdapter() {
                public void focusGained(FocusEvent e) {
                   setFocusFlag(true);
                }
                public void focusLost(FocusEvent e) {
                   setFocusFlag(false);
                }
            };
        }
    }
    public static void setEditObj(VObjIF obj) {
        if (obj != null) {
            if (!((JComponent)obj).isShowing())
                 return;
        }
        newObject=false;
        
        cursor = Cursor.DEFAULT_CURSOR;
        //ParamInfo.setEditElement(obj);
        if (editPanel != null) {
            editPanel.remove(dummy2);
            editPanel.remove(dummy);
            editPanel.repaint();
        }
        resetCursor();
        if (editVobj !=null && obj != editVobj) {
            cleanOldObj();
        }
        isDraging = false;
        isResizing = false;
        //if ((obj == null) || obj==editPanel ||  (!isEditing)) {
        if ((obj == null) ||  (!isEditing)) {
            if (editPanel != null) {
                editPanel.removeKeyListener(kl);
                editPanel.removeFocusListener(fl);
	        }
	        editPanel = pastePanel = null;
	        return;
        }
        if (obj instanceof VGroup && ((VGroup)obj).isLayeredGroup())
            bLayeredGroup = true;
        else
            bLayeredGroup = false;
        
        //setAdapters();
        
        if (frame == null) {
           frame =  (JFrame)VNMRFrame.getVNMRFrame();
           glassPan = frame.getGlassPane();
//         peer = frame.getPeer();
           defCursor = frame.getCursor();
        }
        appIf = Util.getAppIF();

        Container pp = (JComponent)obj;
        //Container pp = ((JComponent)obj).getParent();
        while (pp != null) {
           if (pp instanceof ParamPanel) {
                break;
           }
           pp = pp.getParent();
        }
       // if(obj==editPanel)
        //   editObj=null;
        if((pp == null) || !pp.equals(editPanel)) {
           if (editPanel != null) {
                editPanel.removeKeyListener(kl);
                editPanel.removeFocusListener(fl);
           }
           editPanel = pastePanel = null;
        }
        // (pp == null)
        //   return;
        if (g != null) {
           g.dispose();
           g = null;
        }
        getGraphics = false;
        if (pp!=null && !pp.equals(editPanel)) {
           editPanel = pastePanel= (ParamPanel) pp;
           ParamInfo.setEditPanel(editPanel,false);
           pp.addKeyListener(kl);
           pp.addFocusListener(fl);
        }
        ParamInfo.setEditElement(obj);
        if (pp == null)
            return;        
        g = pp.getGraphics();
        setViewport(pp);

        if (obj !=null && obj != editVobj) {
            editVobj = obj;
           	editObj = (JComponent) obj;
           	editVobj.setEditStatus(true);
           	cursorObj = editObj.getTopLevelAncestor();
           	//addMouseListener(editObj);
        }

        //Dimension  psize = editObj.getSize();
        Dimension  psize = editObj.getPreferredSize();
        Point  pt = editObj.getLocation();
        x = pt.x;
        y = pt.y;
        w = psize.width;
        h = psize.height;
        x2 = -1;

        Point  pt1 = editObj.getLocationOnScreen();
        Point  pt2 = editPanel.getLocationOnScreen();
        Rectangle  rec = editObj.getBounds();
        rec.x = pt1.x - pt2.x;
        rec.y = pt1.y - pt2.y;
        dummy2.setPreferredSize(psize);
        dummy2.setBounds(rec);
        dummy.setBounds(rec);
        dummy2.setVnmrIF(editVobj.getVnmrIF());
        editPanel.add(dummy2, 0);

         //editObj.requestFocus(); // important for edit key listeners
      //dummy2.changeFocus(true);
        dummy2.requestFocus();
        editPanel.repaint();

        if (obj !=null) {
           	newObject=true;
        }

/*
        if (dragSource == null) {
            dragSource = new DragSource();
            dragListener = new VObjDragListener(dragSource);
            dragRecognizer = dragSource.createDefaultDragGestureRecognizer(null,
                          DnDConstants.ACTION_COPY, dragListener);
        }
        if(dragRecognizer != null)
            dragRecognizer.setComponent(editObj);
*/
    } // setEditObj()

    public static void setEditObj2(VObjIF obj) {
        // VText includes VTextWin, and VTextWin needs mouseListener too
        if (editObj2 != null) {
/*
            removeMouseListener(editObj2);
*/
            editObj2.setCursor(null);
            editObj2 = null;
        }
        if (editVobj2 != null) {
            editVobj2.setEditStatus(false);
            editVobj2 = null;
        }
        if (!isEditing)
            return;
        editVobj2 = obj;
        editObj2 = (JComponent) obj;
/*
        addMouseListener(editObj2);
*/
    }

    public static void cleanOldObj() {
        if (editObj != null) {
/*
            removeMouseListener(editObj);
*/
            editObj = null;
        }
        if (editObj2 != null) {
/*
            removeMouseListener(editObj2);
*/
            editObj2 = null;
        }
        if (editVobj != null) {
            editVobj.setEditStatus(false);
            editVobj = null;
        }
        if (editVobj2 != null) {
            editVobj2.setEditStatus(false);
            editVobj2 = null;
        }
    }


    public static void unParent() {
        if (editObj == null || editObj==editPanel)
            return;
        Container pp = editObj.getParent();
        if(pp == editPanel)
            return;
        pp.remove(editObj);
        editPanel.add(editObj);
        setDebug("unParent");
    }

    public static void relocateObj(VObjIF obj) {
        if (obj == null)
            return;
        setDebug("relocateObj");
        editVobj = obj;
        editObj = (JComponent) obj;
        editVobj.setEditStatus(true);
        Dimension  psize = editObj.getPreferredSize();
        Point  pt = editObj.getLocation();
        x2 = pt.x;
        y2 = pt.y;
        w = psize.width;
        h = psize.height;
        difX = 0;
        difY = 0;
        snapIn();
        reParent();
        if (editVobj instanceof VGroupIF)
           reGroup();
        ParamInfo.setEditElement(editVobj);
        relocateDummy();
		setChangedPage(true);
    }

    public static void relocateDummy() {
        if (editPanel != null){
            editPanel.remove(dummy2);
        }
        if ((editObj == null) || (editPanel == null))
            return;
        if (!editObj.isShowing())
            return;
        Dimension  psize = editObj.getPreferredSize();
       // Dimension  psize = editObj.getSize();
        Point  pt1 = editObj.getLocationOnScreen();
        Point  pt2 = editPanel.getLocationOnScreen();
        Rectangle  rec = editObj.getBounds();
        rec.x = pt1.x - pt2.x;
        rec.y = pt1.y - pt2.y;
        dummy2.setBounds(rec);
        dummy2.setPreferredSize(psize);
        editPanel.add(dummy2, 0);

//      editVobj.changeFocus(true);
        dummy2.changeFocus(true);
//      editPanel.validate();
    }

    private static Component getVTarget(Container p, int x, int y) {
        int ncount = p.getComponentCount();
        Component ncomp = null;
        for (int i = 0; i < ncount; i++) {
            Component comp = p.getComponent(i);
            if ((comp != null) && (comp.contains(x - comp.getX(), y - comp.getY())) && comp.isVisible()) {
                if (comp instanceof VObjIF) {
                    EventListener[] elist = comp.getListeners(MouseListener.class);
                    if ((elist != null) && (elist.length > 0))
                        ncomp = comp;
                }
                if (comp instanceof Container) {
                    Container child = (Container) comp;
                    Component deeper = getVTarget(child, x - child.getX(), y - child.getY());
                    if ((deeper != null) && (deeper instanceof VObjIF)) {
                        ncomp = deeper;
                    }
                }
            }
            if (ncomp != null)
                break;
        }
        return ncomp;
    }

    public static void selectNextVobj(MouseEvent e) {
        if (editObj == null || editPanel==null)
            return;
        int ncomponents = editPanel.getComponentCount();
        if (ncomponents <= 0)
            return;
        VObjIF newVobj = null;
        Point pt = dummy2.getLocation();
        int x = e.getX() + pt.x;
        int y = e.getY() + pt.y;
        for (int i = 0 ; i < ncomponents ; i++) {
            Component comp = editPanel.getComponent(i);
            if (comp == dummy2)
                continue;
            if ((comp != null) && (comp.contains(x - comp.getX(), y - comp.getY())) && comp.isVisible()) {
                if (comp instanceof VObjIF) {
                    EventListener[] elist = comp.getListeners(MouseListener.class);
                    if ((elist != null) && (elist.length > 0))
                        newVobj = (VObjIF) comp;
                }
                if (comp instanceof Container) {
                    Container child = (Container) comp;
                    Component deeper = getVTarget(child, x - child.getX(), y - child.getY());
                    if ((deeper != null) && (deeper instanceof VObjIF)) {
                        newVobj = (VObjIF) deeper;
                    }
                }
            }
            if (newVobj != null)
                break;
        }
        if (editVobj == newVobj)
            return;
        if (newVobj == null) {
            setEditObj(newVobj);
            return;
        }
        if ((editVobj == newVobj) || (newVobj == null))
            return;
        Component obj = (Component) newVobj;
        EventListener[] mls = obj.getListeners(MouseListener.class);
        if (mls == null)
            return;

        MouseEvent ne = new MouseEvent(obj, MouseEvent.MOUSE_CLICKED,
                0, e.getModifiers(), 1, 1, 2, false);
        for (int k = 0; k < +mls.length; k++) {
            MouseListener ms = (MouseListener) mls[k];
            if (ms != null) {
                ms.mouseClicked (ne);
            }
        }
    }

    private static void cleanListeners (JComponent obj) {
        int nmembers = obj.getComponentCount();
        obj.removeMouseListener(ml);
        for (int i = 0; i < nmembers; i++) {
            Component comp = obj.getComponent(i);
            if (comp instanceof VGroupIF) {
                cleanListeners((JComponent) comp);
            } else if (comp instanceof VObj) {
                ((VObj)comp).removeMouseListeners(ml, null);
            } else {
                comp.removeMouseListener(ml);
                comp.removeMouseMotionListener(mvl);
            }
        }
    }

    private static void addListeners (JComponent obj) {
        int nmembers = obj.getComponentCount();
        obj.addMouseListener(ml);
        for (int i = 0; i < nmembers; i++) {
            Component comp = obj.getComponent(i);
            if (comp instanceof VGroupIF) {
                addListeners((JComponent) comp);
            } else if (comp instanceof VObj) {
                ((VObj)comp).addMouseListeners(ml, null);
            } else {
                comp.addMouseListener(ml);
                comp.addMouseMotionListener(mvl);
            }
        }
    }

    private static void setFocusFlag(boolean s) {
        if (editObj != null)
            editVobj.changeFocus(s);
        dummy2.changeFocus(s);
    }

	public void setEditMode(boolean s) {
//    	if(parameditutil==null)
//    		parameditutil=new ParamEditUtil();
		isEditing = s;
		dummy2.setEditMode(s);
		if (!s) {
			setEditObj(null);
			if (tmpFile != null && tmpFile.exists())
				tmpFile.delete();
		} else
			appIf = Util.getAppIF();
	}

    private static void mPress (MouseEvent e) {
        Point pt, pt2;
        if (!isEditing || (editObj == null) || editPanel==null) {
            if (editPanel != null){
                editPanel.remove(dummy2);
            }
            return;
        }
        if (appIf == null)
            return;
        InputEvent ine = (InputEvent) e;
        isCopying = false;
        isResizing = false;
        isDraging = false;
        inTrashCan = false;
        isOutside = false;
        outOfBound = false;
        inNewPanel = false;

        repeatEvent = null;
        
        //System.out.println("mPress");
        if (ine.isControlDown() || ine.isShiftDown() ) { // control or shift key pressed
            isDraging = true;
            isCopying = true;
        }
        else {
            if (cursor == Cursor.DEFAULT_CURSOR)
                isDraging = true;
            else
                isResizing = true;
        }
        if (isDraging) {
            if (viewPort == null) {
                viewPoint=new Point(0,0);
                //isDraging = false;
                //isCopying = false;
                //return;
            }
            else
                viewPoint = viewPort.getViewPosition();
            panelBound = editPanel.getBounds();
        }

        Dimension  psize = editObj.getPreferredSize();
        pt = editObj.getLocationOnScreen();
        pt2 = appIf.getLocationOnScreen();
        difAX = pt.x - pt2.x;
        difAY = pt.y - pt2.y;
        pt2 = editPanel.getLocationOnScreen();
        rx = e.getX();
        ry = e.getY();
        w = psize.width;
        h = psize.height;
        x2 = -1;
        y2 = -1;
        mw = 0;
        difX = pt.x - pt2.x;
        difY = pt.y - pt2.y;
        x = difX;
        y = difY;
//      g.setXORMode(Global.BGCOLOR);
        g.setXORMode(Util.getBgColor());

        if (isDraging) {
            x2 = x;
            y2 = y;
            maxH = panelBound.height;
            maxW = panelBound.width;
            if(scrlPanel!=null){
                scrlBound = scrlPanel.getBounds();
                maxH += scrlBound.height - 2;
                maxW += scrlBound.width - 2;
            }
            else
                scrlBound = editPanel.getBounds();

            pt = appIf.getLocationOnScreen();
            if(scrlPanel!=null)
                pt2 = scrlPanel.getLocationOnScreen();            
            else
                pt2 = editPanel.getLocationOnScreen();

            
            panX = pt2.x - pt.x;
            panY = pt2.y - pt.y;
            panX2 = panX + scrlBound.width;
            panY2 = panY + scrlBound.height;

            if (dummy2 != null) {
                editPanel.add(dummy, 0);
            }
            if (cursorObj == null) {
                if (glassPan != null) {
                     glassPan.setVisible(true);
                }
            }
            if (isCopying)
                 setObjCursor(COPY);
            editPanel.validate();
        }
        else {
            g.setColor(Color.yellow);
        }
        orgX = x2;
        orgY = x2;
    }

    private static void mMove (MouseEvent e) {
        if ((!isEditing) || (editObj == null)) {
            if (editObj != null){
                editObj.remove(dummy2);
            }
            return;
        }
        if(!editObj.isShowing())
            return;
        int newCursor;
        JComponent comp = (JComponent)e.getSource();
        Point pe = comp.getLocationOnScreen();
        Point pc = editObj.getLocationOnScreen();
        int mx = e.getX() + pe.x - pc.x;
        int my = e.getY() + pe.y - pc.y;
        int md = e.getModifiers();
        if ((md & InputEvent.BUTTON1_MASK) != 0) {
            mDrag(e);
            return;
        }
        newCursor = Cursor.DEFAULT_CURSOR;
        if (mx < MGAP && mx > 0) {
           if (my < MGAP && my > 0) {
                newCursor = Cursor.NW_RESIZE_CURSOR;
           }
           else if (my > h - MGAP && my < h) {
                newCursor = Cursor.SW_RESIZE_CURSOR;
           }
           else
                newCursor = Cursor.W_RESIZE_CURSOR;
        }
        else if (mx > w - MGAP && mx < w ) {
           if (my < 5 && my > 0) {
                newCursor = Cursor.NE_RESIZE_CURSOR;
           }
           else if (my > h - MGAP && my < h) {
                newCursor = Cursor.SE_RESIZE_CURSOR;
           }
           else
                newCursor = Cursor.E_RESIZE_CURSOR;
        }
        else if (my < MGAP && my > 0) {
                newCursor = Cursor.N_RESIZE_CURSOR;
        }
        else if (my > h - MGAP && my < h) {
                newCursor = Cursor.S_RESIZE_CURSOR;
        }
        if (newCursor != cursor) {
            cursor = newCursor;
            setObjCursor(Cursor.getPredefinedCursor(cursor));
        }
    }

    // used for menu-copy and menu-cut operations
    private static void copyVobj() {
        try {
        	if (tmpFile!=null && tmpFile.exists())
        		tmpFile.delete();
        	tmpFile = File.createTempFile("edit", ".xml");
        }
        catch (IOException e) {
            Messages.writeStackTrace(e);
            return;
        }
        String f = tmpFile.getAbsolutePath();
        VObjIF obj=editVobj;
        String r=obj.getAttribute(USEREF); // expand all clip object references
        obj.setAttribute(USEREF,"false");
        LayoutBuilder.writeToFile((JComponent)obj, f);
        obj.setAttribute(USEREF,r);
		clipempty=false;

        StringSelection data = new StringSelection(tmpFile.getName());
        clipboard.setContents(data, data);
    }

    // used for menu-paste operation
    private static void pasteVobj(ParamPanel panel,Point p) {
    	if(tmpFile!=null){
	        String f = tmpFile.getAbsolutePath();
	        panel.pasteXmlObj(f, p.x, p.y);
			setChangedPage(true);
    	}
    }

    private static void pasteVobj() {
    	//System.out.println("pasteVobj");

        // String f = File.separator+"tmp"+File.separator+"edititem.xml";
        File fd;
        try {
            fd = File.createTempFile("edit", ".xml");
        }
        catch (IOException e) {
            Messages.writeStackTrace(e);
            return;
        }
        String f = fd.getAbsolutePath();
        LayoutBuilder.writeToFile((JComponent)editVobj, f);
        // File fd = new File(f);
        if (!fd.exists()) {
            return;
        }
        editPanel.pasteXmlObj(f, x2, y2);
        fd.delete();
		setChangedPage(true);
    }


	private static void setObjCursor(Cursor c){
		if (c == null) {
			dummy2.setCursor(defCursor);
			dummy.setCursor(defCursor);
			if (cursorObj != null){
				cursorObj.setCursor(defCursor);
			}
		} else {
			dummy.setCursor(c);
			dummy2.setCursor(c);
			if (cursorObj != null){
				cursorObj.setCursor(c);
			}
			
//			if (cursorObj != null) {
//				cursorObj.setCursor(c);
//				editObj.setCursor(c);
//			} else {
//				editObj.setCursor(c);
//				if ((glassPan != null) && (glassPan.isVisible())) {
//					glassPan.setCursor(c);
//				}
//			}			 
		}
	}

    private static void setObjCursor(int c) {
        if (c == cursor)
            return;
        Cursor xcursor = null;
        switch (c) {
          case  MOVE:
                    xcursor = moveCursor;
                    break;
          case  OUT:
                    xcursor = outCursor;
                    break;
          case  COPY:
                    xcursor = copyCursor;
                    break;
          default:
                    xcursor = defCursor;
                    break;
        }
        cursor = c;
        //System.out.println("setObjCursor:"+c);
        setObjCursor(xcursor);
    }

    public static void removeVobj() {
        if (editVobj == null)
            return;
        setDebug("removeVobj");

        JComponent pobj = (JComponent) editObj.getParent();
        ParamInfo.setItemEdited(editVobj);
        editVobj.destroy();
        pobj.remove((JComponent)editVobj);
        if(ParamPanel.isTabGroup((Component)editVobj)){
            editPanel.removeTab(editVobj);
        }
        else if(pobj instanceof VObjIF)
            setEditObj((VObjIF)pobj);
        else
            setEditObj(null);

        if (editVobj2 != null)
            editVobj2.destroy();
        editVobj2 = null;
        
		setChangedPage(true);
        pobj.repaint();
    }

    private static synchronized void dummyRelease (MouseEvent e) {
        if (!isDraging)
            return;
        isDraging = false;
        if (editObj == null)
            return;

        if (inTrashCan) {
            if (!isCopying) { // delete obj
                removeVobj();
            }
            return;
        }
        if (outOfBound) {
            if(editPanel!=pastePanel){
            	Point srcPoint=e.getPoint(); // coordinates in source panel 
            	SwingUtilities.convertPointToScreen(srcPoint, e.getComponent());
            	SwingUtilities.convertPointFromScreen(srcPoint, pastePanel);
            	pastePoint=srcPoint;
            	copyObject();
            	if (!isCopying){
                    Container  pp=editObj.getParent();
                    if(!(pp instanceof JTabbedPane)){
                        pp.remove(editObj);
                    }
            	}
            	pasteObject();
            	dummy.setVisible(true);
            }
            return;
        }
        if (isCopying) {
           if (x2 >= 0 && y2 >= 0) {
                if (x2 != x || y2 != y)
                   pasteVobj();
                else
                   editVobj.setEditStatus(true);
                ParamInfo.getObjectGeometry();
           }
           return;
        }
        else { // move component
           if (orgX == x2 && orgY == y2) {
                return;
           }
           Point pt = editPanel.getLocationOnScreen();
           Container  pp=editObj.getParent();
           if(!(pp instanceof JTabbedPane)){
               pp.remove(editObj);
               editPanel.add(editObj);
           }
           editObj.setLocation(pt.x + x2, pt.y + y2);
           relocateObj(editVobj);
           ParamInfo.getObjectGeometry();
        }
    }

    private static void mRelease (MouseEvent e) {
        if (!isEditing || editObj == null || editPanel==null) {
            if (editPanel != null) {
                editPanel.remove(dummy2);
                editPanel.remove(dummy);
            }
            return;
        }
        if(dummy != null)
            editPanel.remove(dummy);
        setObjCursor(0);
        if (isDraging) {
            dummyRelease(e);
            if (editPanel != null)
               editPanel.repaint();
            return;
        }
        if (!isResizing) {
           return;
        }
        if (x2 >= 0)
           g.drawRect(x2, y2, w, h);
        g.setPaintMode();
        if (x2 < 0) {
           x2 = -1;
           isResizing = false;
           return;
        }
        boolean changeGrp = false;
        if (editVobj instanceof VGroupIF) {
                if (mw != w || mh != h)
                    changeGrp = true;
           }
        w = mw;
        h = mh;
        if (y2 < 0) {
           h = h + y2;
           y2 = 0;
        }
        boolean bGroup = false;
        if (editVobj instanceof VGroup)
        {
        	// DS 9/3/09: this call causes the group's size to always be a multiple
        	// of 70 pixels (thus preventing accurate resizing using the mouse)
        	// I'm not sure what the original purpose was (history lost from sccs) 
        	// but I've commented the call out in order to to make resizing work.
            //w = ((VGroup)editVobj).getWidthtype(w);
            if (((VGroup)editVobj).getLayerGroups() > 0)
                bGroup = true;
        }
        if (w < minSize)
           w = minSize;
        if (h < minSize)
           h = minSize;
        //snapIn();

        if (changeGrp && !(editObj instanceof VParameter) && !bGroup)
           releaseGroupMember(x2, y2, w, h);
        x = x2;
        y = y2;
        Point pt = editPanel.getLocationOnScreen();
        x2 += pt.x;
        y2 += pt.y;
        snapIn();
        reParent();
        if (editVobj instanceof VGroupIF)
           reGroup();
        x2 = -1;
        if(isResizing && editObj!=null){
                editObj.validate();
//              editObj.requestFocus();
        }
        isResizing = false;
        ParamInfo.getObjectGeometry();
    }

    public static void paintPanel(Graphics gr) {
        if (!isDraging || isOutside) {
           return;
        }
        if (editPanel != null) {
           gr.setColor(Color.red);
           gr.drawRect(1, 1, w, h);
        }
    }

    private static void startTimer() {
        if (timer == null)
            timer = new Timer(100, dummy);
        if (timer != null && (!timer.isRunning()))
            timer.restart();
    }

    private static void stopTimer() {
        if (timer != null)
            timer.stop();
    }

    public static void repeatDrag() {
        if (!isDraging || repeatEvent == null) {
            stopTimer();
            return;
        }
        if (!isOutside)
            cpDrag(repeatEvent, true);
    }

    private static synchronized void cpDrag (MouseEvent e, boolean fromRepeat) {
    	boolean globalcursors=false;
        repeatEvent = null;
        int nx = e.getX();
        int ny = e.getY();
        int oldy = y2;
        int oldx = x2;
        x2 = x + nx - rx;
        y2 = y + ny - ry;
        int dx = x2 + w;
        int dy = y2 + h;
        int vx = nx + difAX;
        int vy = ny + difAY;
        boolean change = false;
        boolean change2 = false;
        boolean repeat = false;
        int mvx = x2 - oldx;
        int mvy = y2 - oldy;
        if (mvx == 0 && mvy == 0)
            return;

        if (!fromRepeat) {
           if (vx < panX || vx > panX2 || vy < panY || vy > panY2) {
              Component comp = getTrashTarget(appIf, vx, vy);
              if (comp != null) {
                   if (!inTrashCan && !isCopying) {
                        inTrashCan = true;
                        setObjCursor(MOVE);
                   }
              }
              else {
                   if (inTrashCan) {
                        setObjCursor(OUT);
                        inTrashCan = false;
                   }
              }
              if (!isOutside) {
                if (!inTrashCan) {
                   //setObjCursor(OUT);
                }
                isOutside = true;
              }
              if(inNewPanel){
            	  Point p=e.getPoint();
              	  SwingUtilities.convertPointToScreen(p, e.getComponent());
            	  SwingUtilities.convertPointFromScreen(p, pastePanel);
            	  if(globalcursors && p.x>0 && p.x<pastePanel.getWidth() && p.y>0 && p.y<pastePanel.getHeight()){
            		  // Get here ok when mouse enters a ParamPanel panel different from drag source panel
            		  // Now, want to change the cursor to show "drop/copy allowed" but
            		  // - The cursor never changes 
            		  // - Nothing seems to work (including the code below)
            		  // TODO: fix this problem
            		  
            		  RootPaneContainer root = 
            		      (RootPaneContainer)pastePanel.getTopLevelAncestor();
            		  Component glassPan = root.getGlassPane();
            		  if(isCopying)
            			  glassPan.setCursor(copyCursor);
            		  else
            			  glassPan.setCursor(moveCursor);
            		  glassPan.setVisible(true);
            	  }
              }
              outOfBound = true;
              return;
           }
           isOutside = false;
        } // not formRepeat
        if (x2 < 0 || y2 < 0) {
           if (!outOfBound) {
                outOfBound = true;
                dummy.setVisible(false);
                // temporary workaround for above cursor problem is to not
                // change the cursor to "drop prohibited" when mouse leaves source panel
                // TODO: define globalcursors when cursor problem fixed
                if(globalcursors)
                	setObjCursor(OUT);
           }
        }
        else if (outOfBound) {
           dummy.setVisible(true);
           outOfBound = false;
           isOutside = false;
           if (isCopying)
                setObjCursor(COPY);
           else
                setObjCursor(0);
        }
        if (!fromRepeat) {
           if (mvx > 60 || mvx < -60 || mvy > 60 || mvy < -60) {
              // move fast
              dummy.setBounds(x2, y2, w+2, h+2);
              return;
           }
        }
        if (mvx < 0 && viewPoint.x > 0) { // move left
            vx = x2 - viewPoint.x;
            if (vx < 6) {
                mvx = 0 - mvx;
                vx = oldx - viewPoint.x;
                if (vx > 0) {
                   viewPoint.x = x2 - vx;
                }
                else {
                   if (mvx < 10)
                        mvx = 10;
                   viewPoint.x = viewPoint.x - mvx;
                }
                if (viewPoint.x < 0)
                   viewPoint.x = 0;
                else {
                   repeat = true;
                   vx = scrlBound.width / 10;
                   if (vx > viewPoint.x)
                        vx = viewPoint.x;
                   e.translatePoint(-vx, 0);
                }
                change = true;
            }
        }
        else if (autoscroll && mvx > 0  && x2 > (viewPoint.x + 2)) { // move right
           vx = viewPoint.x + scrlBound.width;
           if (vx < maxW) {
               vy = vx - dx;
               if (vy < 6) {
                   vy =  vx - oldx - w;
                   if (vy > 0) {
                        viewPoint.x = viewPoint.x + mvx;
                   }
                   else {
                        if (mvx < 8)
                            mvx = 8;
                        viewPoint.x += mvx;
                   }
                   if (viewPoint.x + scrlBound.width > maxW)
                        viewPoint.x = maxW - scrlBound.width;
                   else {
                      repeat = true;
                      vx = scrlBound.width / 10;
                      if (vx > 30)
                           vx = 30;
                      e.translatePoint(vx, 0);
                   }
                   change = true;
                }
           }
        }
        if (mvy < 0 && viewPoint.y > 0) { // move up
           vy = y2 - viewPoint.y;
           if (vy < 6) {
                vx = oldy - viewPoint.y;
                if (vx > 0) {
                    viewPoint.y = y2 - vx;
                }
                else {
                    if (mvy > -8)
                           mvy = -8;
                    viewPoint.y = viewPoint.y + mvy;
                }
                if (viewPoint.y < 0)
                    viewPoint.y = 0;
                else {
                    repeat = true;
                    vx = scrlBound.height / 10;
                    if (vx > viewPoint.y)
                        vx = viewPoint.y;
                    e.translatePoint(0, -vx);
                }
                change = true;
           }
        }
        else if (autoscroll &&  mvy > 0  && y2 > (viewPoint.y + scrlBound.height * 0.6)) { // move down
            vy = viewPoint.y + scrlBound.height;
            vx = vy - dy;
            if ((vy < maxH) && (vx < 6)) {
                if (vx > 3) {
                    if (mvy < 3)
                        mvy = 3;
                }
                else {
                    if (mvy < 8)
                        mvy = 8;
                }
                viewPoint.y = viewPoint.y + mvy;
                change = true;
                vy = viewPoint.y + scrlBound.height;
                if (vy > maxH)
                    viewPoint.y = maxH - scrlBound.height;
                else {
                    repeat = true;
                    vx = scrlBound.height / 10;
                    if (vx > 30)
                        vx = 30;
                    e.translatePoint(0, vx);
                }
            }
        }
        if (change) {
            if (dx > panelBound.width && panelBound.width < maxW) {
                panelBound.width = dx + 2;
                change2 = true;
            }
            if (dy > panelBound.height && panelBound.height < maxH) {
                panelBound.height = dy + 2;
                change2 = true;
            }
            dummy.setBounds(x2, y2, w+2, h+2);
            if (change2) {
                editPanel.setPreferredSize(new Dimension(panelBound.width, panelBound.height));
                if(viewPort!=null)
                    viewPort.setViewSize(new Dimension(panelBound.width, panelBound.height));
            }
            if(viewPort!=null)
                viewPort.setViewPosition(viewPoint);
            Point ptx = editObj.getLocationOnScreen();
            Point pta = appIf.getLocationOnScreen();
            difAX = ptx.x - pta.x;
            difAY = ptx.y - pta.y;
            if (repeat) {
                repeatEvent = e;
                startTimer();
            }
            else {
                stopTimer();
            }
            getGraphics = true;
            return;
        }
        dummy.setBounds(x2, y2, w+2, h+2);
    }

    private static void mDrag (MouseEvent e) {
    	setChangedPage(true);
        if (isResizing) {
            mResize(e);
            return;
        }
        if (isDraging) {
            cpDrag(e, false);
            return;
        }
    }

    private static void keyMove (int key) {
        if (editObj == null)
            return;
        if (!editObj.isShowing())
            return;
        Point pt = editObj.getLocationOnScreen();
        int gap = 1;
        if (snapFlag)
            gap = snapGap;
        x2 = pt.x;
        y2 = pt.y;
        if (key == KeyEvent.VK_LEFT)
            x2 = x2 - gap;
        else if (key == KeyEvent.VK_RIGHT)
            x2 = x2 + gap;
        else if (key == KeyEvent.VK_UP)
            y2 = y2 - gap;
        else if (key == KeyEvent.VK_DOWN)
            y2 = y2 + gap;
        else
            return;
        Point pt2 = editPanel.getLocationOnScreen();
        if (x2 < pt2.x)
            x2 = pt2.x;
        if (y2 < pt2.y)
            y2 = pt2.y;
        snapIn();
        reParent();
        if (editVobj instanceof VGroupIF)
           reGroup();
        ParamInfo.getObjectGeometry();
		setChangedPage(true);
		if(editPanel.getParameterPanel()==null){ // adjust scrollbars if needed
			editPanel.adjustSize();
			editPanel.validate();
		}
    }

    private static void keyResize (int key) {
        if (editObj == null)
            return;
        if (!editObj.isShowing())
            return;
        Point pt = editObj.getLocationOnScreen();
        int gap = 1;
        if (snapFlag)
            gap = snapGap;
        x2 = pt.x;
        y2 = pt.y;
        if (key == KeyEvent.VK_LEFT) {
            w -= gap;
        }
        else if (key == KeyEvent.VK_RIGHT) {
            w += gap;
        }
        else if (key == KeyEvent.VK_DOWN) {
            h += gap;
        }
        else if (key == KeyEvent.VK_UP) {
            h -= gap;
        }
        else
            return;
        if (w < minSize)
            w = minSize;
        if (h < minSize)
            h = minSize;
        snapIn();
        reParent();
        if (editVobj instanceof VGroupIF)
           reGroup();
        ParamInfo.getObjectGeometry();
		setChangedPage(true);
    }

    private static void snapIn () {
        if (!snapFlag) {
            return;
        }
        if (snapGap < 2)
            return;
        Point pt = editPanel.getLocationOnScreen();
        int dx = x2 - pt.x;
        int dy = y2 - pt.y;
        
        int d1 = dx / snapGap;
        int d2 = dx % snapGap;
        if (d2 > snapGap / 2)
            d1++;
        dx = d1 * snapGap;
        d1 = dy / snapGap;
        d2 = dy % snapGap;
        if (d2 > snapGap / 2)
            d1++;
        dy = d1 * snapGap;

        d1 = (w + snapGap / 2) / snapGap;
        if (d1 < 1)
            d1 = 1;
        w = d1 * snapGap;
        d1 = (h + snapGap / 2) / snapGap;
        if (d1 < 1)
            d1 = 1;
        h = d1 * snapGap;
        difX=x2+dx;
        difY=y2+dy;

        x2 = pt.x + dx;
        y2 = pt.y + dy;

    }

    private static void mResize (MouseEvent e) {
        int nx = e.getX();
        int ny = e.getY();

        if (x2 >= 0){
           // System.out.println("old:"+rect.toString());
            g.drawRect(x2, y2, mw, mh);
        }
        switch (cursor) {
           case Cursor.N_RESIZE_CURSOR:
                mw = w;
                x2 = x;
                y2 = y + ny - ry;
                mh = h + y - y2;
                if (mh < 0) {
                   mh = 0 - mh;
                   y2 = y + h;
                }
                break;
           case Cursor.W_RESIZE_CURSOR:
                x2 = x + nx - rx;
                y2 = y;
                mh = h;
                mw = w + x - x2;
                if (mw < 0) {
                   x2 = x + w;
                   mw = 0 - mw;
                }
                break;
           case Cursor.S_RESIZE_CURSOR:
                x2 = x;
                y2 = y;
                mw = w;
                mh = h + ny - ry;
                if (mh < 0) {
                   y2 = y + mh;
                   mh = 0 - mh;
                }
                break;
           case Cursor.E_RESIZE_CURSOR:
                x2 = x;
                y2 = y;
                mh = h;
                mw = w + nx - rx;
                if (mw < 0) {
                   x2 = x + mw;
                   mw = 0 - mw;
                }
                break;
           case Cursor.NW_RESIZE_CURSOR:
                y2 = y + ny - ry;
                mh = h + y - y2;
                if (mh < 0) {
                   mh = 0 - mh;
                   y2 = y + h;
                }
                x2 = x + nx - rx;
                mw = w + x - x2;
                if (mw < 0) {
                   x2 = x + w;
                   mw = 0 - mw;
                }
                break;
           case Cursor.SW_RESIZE_CURSOR:
                x2 = x + nx - rx;
                mw = w + x - x2;
                if (mw < 0) {
                   x2 = x + w;
                   mw = 0 - mw;
                }
                y2 = y;
                mh = h + ny - ry;
                if (mh < 0) {
                   y2 = y + mh;
                   mh = 0 - mh;
                }
                break;
           case Cursor.NE_RESIZE_CURSOR:
                y2 = y + ny - ry;
                mh = h + y - y2;
                if (mh < 0) {
                   mh = 0 - mh;
                   y2 = y + h;
                }
                x2 = x;
                mw = w + nx - rx;
                if (mw < 0) {
                   x2 = x + mw;
                   mw = 0 - mw;
                }
                break;
           case Cursor.SE_RESIZE_CURSOR:
                y2 = y;
                mh = h + ny - ry;
                if (mh < 0) {
                   y2 = y + mh;
                   mh = 0 - mh;
                }
                x2 = x;
                mw = w + nx - rx;
                if (mw < 0) {
                   x2 = x + mw;
                   mw = 0 - mw;
                }
                break;
        }
        g.drawRect(x2, y2, mw, mh);
    }

    public static ParamPanel getEditPanel() {
        return Util.getParamPanel().getViewPanel();
        //return editPanel;
    }

    public static ParamPanel getEditPanel(JComponent obj) {
    	if(obj instanceof ParamPanel)
    		return (ParamPanel)obj;
	    Container pp = ((JComponent)obj).getParent();
	    while (pp != null) {
	       if (pp instanceof ParamPanel) {
	           return (ParamPanel)pp;
	       }
	       pp = pp.getParent();
	    }
	    return null;
    }

    public static void setEditPanel(ParamPanel p) {
        if (p == null) {
            if (editObj != null)
                editVobj.setEditStatus(false);
            if (editObj2 != null)
                editVobj2.setEditStatus(false);
            if (editPanel != null) {
                editPanel.removeKeyListener(kl);
                editPanel.removeFocusListener(fl);
                editPanel.remove(dummy2);
                editPanel.remove(dummy);
            }
        }
        editPanel = null;
        //editPanel = p;
   }

    private static JComponent newParent(int deep, JComponent p) {
        int nmembers = p.getComponentCount();
        JComponent newp = null;
        for (int i = 0; i < nmembers; i++) {
            Component comp = p.getComponent(i);
            if (comp instanceof VGroupIF) {
                JComponent nobj = (JComponent) comp;
                if (!nobj.equals(editObj)) {
                        newp = newParent(deep+1, nobj);
                        if (newp != null) {
                           p = newp;
                           break;
                        }
                }
            }
        }
        if(!p.isShowing())
            return null;
        Rectangle rect = p.getBounds();
        Point pt = p.getLocationOnScreen();
        Rectangle par_rect=new Rectangle(pt.x,pt.y,rect.width,rect.height);
        Rectangle obj_rect=new Rectangle(x2,y2,w,h);
        if(obj_rect.equals(par_rect)) // target and new object same size
        	return null;
        else if(par_rect.contains(obj_rect)) // target group contains new object
        	return p;
        else // target doesn't contain new object
        	return null;
    }

    public static void reParent() {
        Point pt2;

        Container pp = editObj.getParent();

        int nmembers = editPanel.getComponentCount();
        JComponent np = null;
        for (int i = 0; i < nmembers; i++) {
            Component comp = editPanel.getComponent(i);
            if (comp instanceof VGroupIF  && comp.isShowing()) {
                JComponent p = (JComponent) comp;
                if (!p.equals(editObj)) {
                    JComponent p2 = newParent(1, p);
                    if (p2 != null) {
                        np = p2;
                        break;
                    }
                }
            }
        }
        editObj.setPreferredSize(new Dimension(w, h));
        if (np != null) {
            if (!(pp instanceof JTabbedPane)) {
                pp.remove(editObj);
                if (editVobj instanceof VGroupIF)
                    np.add(editObj, 0);
                else
                    np.add(editObj, 0);
                pp.validate();
                np.validate();
                pp.repaint();
            }
            pt2 = np.getLocationOnScreen();
            editObj.setBounds(x2 - pt2.x, y2 - pt2.y, w, h);
            np.repaint();
        } else {
            if (!(pp instanceof JTabbedPane)) {
                pp.remove(editObj);
                if (editVobj instanceof VGroupIF)
                    editPanel.add(editObj, 0);
                else
                    editPanel.add(editObj, 0);
                pp.validate();
                editPanel.revalidate();
                pp.repaint();
            }
            pt2 = editPanel.getLocationOnScreen();
            editObj.setBounds(x2 - pt2.x, y2 - pt2.y, w, h);
            editPanel.repaint();
        }
        if (editObj instanceof VScroll || editObj instanceof VText
                || editObj instanceof VSlider) {
            editObj.validate();
        }
        ParamInfo.setItemEdited((VObjIF)editObj);
        setDebug("reParent");
    }

    private static void reGroup() {
        JComponent grp = editObj;
        if (!(editObj instanceof VGroupIF))
            return;
        if(!grp.isShowing())
            return;
        if(editObj instanceof JTabbedPane)
            return;
        Container pp = editObj.getParent();
        if(pp instanceof JTabbedPane)
            return;
        Point pt2;
        Rectangle rect2;
        JComponent p;
        Component comp;
        //Container cont=editPanel;
        Container cont=pp;
        int     px1 = x2;
        int     py1 = y2;
        int     px2 = x2 + w;
        int     py2 = y2 + h;
        int     nmembers = cont.getComponentCount();
        int     i = 0;
        while (i < nmembers) {
           comp = cont.getComponent(i);
           i++;
           if (!comp.isShowing())
               continue;
           if ((comp == editObj) || (comp == pp)) {
               continue;
           }

           if ((comp instanceof VObjIF)) {
                p = (JComponent) comp;
                rect2 = p.getBounds();
                pt2 = p.getLocationOnScreen();                
                if (pt2.x >= px1 && (pt2.x + rect2.width <= px2)) {
                    if (pt2.y >= py1 && (pt2.y + rect2.height <= py2)) {
                       cont.remove(comp);
                       nmembers--;
                       i--;
                       if (p instanceof VGroupIF)
                            grp.add(p);
                       else
                            grp.add(p, 0);
                       p.setBounds(pt2.x - px1, pt2.y - py1, rect2.width, rect2.height);
                    }
                }
           }
        }
        grp.revalidate();
        setDebug("reGroup");

    }

    private static void releaseGroupMember(int x, int y, int w, int h) {
        JComponent p;
        Component  comp;
        Point  pt2;
        if (editObj == null || (!editObj.isShowing()))
             return;
        setDebug("releaseGroupMember");
        Container parent = editObj.getParent();
        if (parent == null)
            parent = editPanel;
        Rectangle rect = editObj.getBounds();
        Point pt0 = parent.getLocationOnScreen();
        int     px1 = x - difX;
        int     px2 = px1 + w;
        int     py1 = y - difY;
        int     py2 = py1 + h;
        int     i = 0;
        int     k = editObj.getComponentCount();
        boolean toRemove;
        while (i < k) {
           toRemove = false;
           comp = editObj.getComponent(i);
           i++;
           if (comp instanceof VObjIF && comp.isShowing()) {
                p = (JComponent) comp;
                rect = p.getBounds();
                pt2 = p.getLocationOnScreen();
                if (rect.x < px1 || (rect.x + rect.width > px2))
                    toRemove = true;
                else if (rect.y < py1 || (rect.y + rect.height > py2))
                    toRemove = true;
                if (toRemove) {
                    editObj.remove(p);
                    parent.add(p);
                    p.setBounds(pt2.x - pt0.x, pt2.y - pt0.y,
                        rect.width, rect.height);
                    k--;
                    i--;
                }
                else {
                    rect.x = rect.x - px1;
                    rect.y = rect.y - py1;
                    p.setBounds(rect.x, rect.y, rect.width, rect.height);
                }
            }
        }
        editObj.revalidate();
        if (parent instanceof JComponent)
            ((JComponent)parent).revalidate();

    } // reGroup

    public static void setSnapGap(int k) {
            snapGap = k;
            if (snapGap < 1)
                snapGap = 1;
    }
    public static int getSnapGap() {
    	return snapGap;
    }
    public static void setSnapGap(String s) {
        s.trim();
        if (s.length() < 1)
           return;
        try {
           snapGap = Integer.parseInt(s);
        }
        catch (NumberFormatException e) { }
        if (snapGap < 1)
           snapGap = 1;
    }

    public static void setSnap(boolean s) {
        if (s) {
           if (!snapFlag) {
           }
        }
        snapFlag = s;
    }

    private static Component getTarget(Container p, int level, int x, int y) {
        int ncomponents = p.getComponentCount();
        for (int i = 0 ; i < ncomponents ; i++) {
            Component comp = p.getComponent(i);
            if ((comp != null) && (comp.contains(x - comp.getX(), y - comp.getY())) && comp.isVisible()) {
                if (comp instanceof TrashCan)
                    return comp;
                if (comp instanceof Container) {
                    Container child = (Container) comp;
                    Component deeper = getTarget(child, level+1, x - child.getX(), y - child.getY());
                    if ((deeper != null) && (deeper instanceof TrashCan)) {
                        return deeper;
                    }
                }
            }
        }
        return null;
    }

    private static Component getTrashTarget(Container p, int x, int y) {
        int ncomponents = p.getComponentCount();
        for (int i = 0 ; i < ncomponents ; i++) {
            Component comp = p.getComponent(i);
            if ((comp != null) && (comp.contains(x - comp.getX(), y - comp.getY())) && (comp.isVisible() == true)) {
                if (comp instanceof TrashCan)
                    return comp;
                if (comp instanceof Container) {
                    Container child = (Container) comp;
                    Component deeper = getTarget(child, 1,  x - child.getX(), y - child.getY());
                    if ((deeper != null) && (deeper instanceof TrashCan)) {
                        return deeper;
                    }
                }
                else
                    return null;
            }
        }
        return null;
    }
}

    class VPseudo extends JComponent implements ActionListener
    {
        public VPseudo() {
            setOpaque(false);
        }

        public void actionPerformed(ActionEvent e) {
            ParamEditUtil.repeatDrag();
        }

        public void paint(Graphics g) {
            ParamEditUtil.paintPanel(g);
        }
    }

	class EMenu extends JPopupMenu implements PropertyChangeListener {
		public EMenu() {
		    DisplayOptions.addChangeListener(this);
			ParamEditUtil.cut = add(Util.getLabel("emCut", "Cut"));
			ParamEditUtil.cut.setAccelerator(KeyStroke.getKeyStroke(java.awt.event.KeyEvent.VK_X,
                    java.awt.Event.CTRL_MASK));
			ParamEditUtil.cut.addActionListener( new ActionListener() {
				public void actionPerformed(ActionEvent evt) {
					ParamEditUtil.cutObject();
				}
			});

			ParamEditUtil.copy = add(Util.getLabel("emCopy", "Copy"));
			ParamEditUtil.copy.setAccelerator(KeyStroke.getKeyStroke(java.awt.event.KeyEvent.VK_C,
                    java.awt.Event.CTRL_MASK));
			ParamEditUtil.copy.addActionListener(new ActionListener() {
				public void actionPerformed(ActionEvent evt) {
					ParamEditUtil.copyObject();
				}
			});

			ParamEditUtil.paste = add(Util.getLabel("emPaste", "Paste"));
			ParamEditUtil.paste.setAccelerator(KeyStroke.getKeyStroke(java.awt.event.KeyEvent.VK_V,
                    java.awt.Event.CTRL_MASK));

			ParamEditUtil.paste.addActionListener(new ActionListener() {
				public void actionPerformed(ActionEvent evt) {
					ParamEditUtil.pasteObject();
				}
			});
			ParamEditUtil.clear = add(Util.getLabel("emClear", "Clear"));
			ParamEditUtil.clear.setAccelerator(KeyStroke.getKeyStroke(java.awt.event.KeyEvent.VK_BACK_SPACE,0));
			//ParamEditUtil.clear.setAccelerator(KeyStroke.getKeyStroke("<bs>"));

			ParamEditUtil.clear.addActionListener(new ActionListener() {
				public void actionPerformed(ActionEvent evt) {
					ParamEditUtil.clearObject();
				}
			});

		}

        @Override
        public void propertyChange(PropertyChangeEvent evt) {
            if(DisplayOptions.isUpdateUIEvent(evt))
                SwingUtilities.updateComponentTreeUI(this);
        }
	}

