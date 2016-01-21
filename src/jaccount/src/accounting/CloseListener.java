/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 * CloseListener.java
 *
 * Created on June 3, 2007, 7:15 AM
 *
 * To change this template, choose Tools | Template Manager
 * and open the template in the editor.
 */

package accounting;

import java.awt.event.MouseEvent;
import java.util.EventListener;


public interface CloseListener extends EventListener {
	public void closeOperation(MouseEvent e);
}
