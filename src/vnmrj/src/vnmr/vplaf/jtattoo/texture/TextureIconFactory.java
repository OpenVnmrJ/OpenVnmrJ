/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 * Copyright 2012 MH-Software-Entwicklung. All rights reserved.
 * Use is subject to license terms.
 */
package vnmr.vplaf.jtattoo.texture;

import vnmr.vplaf.jtattoo.AbstractIconFactory;
import javax.swing.Icon;

/**
 * @author Michael Hagen
 */
public class TextureIconFactory implements AbstractIconFactory {

    private static TextureIconFactory instance = null;

    private TextureIconFactory() {
    }

    public static synchronized TextureIconFactory getInstance() {
        if (instance == null) {
            instance = new TextureIconFactory();
        }
        return instance;
    }

    public Icon getOptionPaneErrorIcon() {
        return TextureIcons.getOptionPaneErrorIcon();
    }

    public Icon getOptionPaneWarningIcon() {
        return TextureIcons.getOptionPaneWarningIcon();
    }

    public Icon getOptionPaneInformationIcon() {
        return TextureIcons.getOptionPaneInformationIcon();
    }

    public Icon getOptionPaneQuestionIcon() {
        return TextureIcons.getOptionPaneQuestionIcon();
    }

    public Icon getFileChooserDetailViewIcon() {
        return TextureIcons.getFileChooserDetailViewIcon();
    }

    public Icon getFileChooserHomeFolderIcon() {
        return TextureIcons.getFileChooserHomeFolderIcon();
    }

    public Icon getFileChooserListViewIcon() {
        return TextureIcons.getFileChooserListViewIcon();
    }

    public Icon getFileChooserNewFolderIcon() {
        return TextureIcons.getFileChooserNewFolderIcon();
    }

    public Icon getFileChooserUpFolderIcon() {
        return TextureIcons.getFileChooserUpFolderIcon();
    }

    public Icon getMenuIcon() {
        return TextureIcons.getMenuIcon();
    }

    public Icon getIconIcon() {
        return TextureIcons.getIconIcon();
    }

    public Icon getMaxIcon() {
        return TextureIcons.getMaxIcon();
    }

    public Icon getMinIcon() {
        return TextureIcons.getMinIcon();
    }

    public Icon getCloseIcon() {
        return TextureIcons.getCloseIcon();
    }

    public Icon getPaletteCloseIcon() {
        return TextureIcons.getPaletteCloseIcon();
    }

    public Icon getRadioButtonIcon() {
        return TextureIcons.getRadioButtonIcon();
    }

    public Icon getCheckBoxIcon() {
        return TextureIcons.getCheckBoxIcon();
    }

    public Icon getComboBoxIcon() {
        return TextureIcons.getComboBoxIcon();
    }

    public Icon getTreeComputerIcon() {
        return TextureIcons.getTreeComputerIcon();
    }

    public Icon getTreeFloppyDriveIcon() {
        return TextureIcons.getTreeFloppyDriveIcon();
    }

    public Icon getTreeHardDriveIcon() {
        return TextureIcons.getTreeHardDriveIcon();
    }

    public Icon getTreeFolderIcon() {
        return TextureIcons.getTreeFolderIcon();
    }

    public Icon getTreeLeafIcon() {
        return TextureIcons.getTreeLeafIcon();
    }

    public Icon getTreeCollapsedIcon() {
        return TextureIcons.getTreeControlIcon(true);
    }

    public Icon getTreeExpandedIcon() {
        return TextureIcons.getTreeControlIcon(false);
    }

    public Icon getMenuArrowIcon() {
        return TextureIcons.getMenuArrowIcon();
    }

    public Icon getMenuCheckBoxIcon() {
        return TextureIcons.getMenuCheckBoxIcon();
    }

    public Icon getMenuRadioButtonIcon() {
        return TextureIcons.getMenuRadioButtonIcon();
    }

    public Icon getUpArrowIcon() {
        return TextureIcons.getUpArrowIcon();
    }

    public Icon getDownArrowIcon() {
        return TextureIcons.getDownArrowIcon();
    }

    public Icon getLeftArrowIcon() {
        return TextureIcons.getLeftArrowIcon();
    }

    public Icon getRightArrowIcon() {
        return TextureIcons.getRightArrowIcon();
    }

    public Icon getSplitterDownArrowIcon() {
        return TextureIcons.getSplitterDownArrowIcon();
    }

    public Icon getSplitterHorBumpIcon() {
        return TextureIcons.getSplitterHorBumpIcon();
    }

    public Icon getSplitterLeftArrowIcon() {
        return TextureIcons.getSplitterLeftArrowIcon();
    }

    public Icon getSplitterRightArrowIcon() {
        return TextureIcons.getSplitterRightArrowIcon();
    }

    public Icon getSplitterUpArrowIcon() {
        return TextureIcons.getSplitterUpArrowIcon();
    }

    public Icon getSplitterVerBumpIcon() {
        return TextureIcons.getSplitterVerBumpIcon();
    }

    public Icon getThumbHorIcon() {
        return TextureIcons.getThumbHorIcon();
    }

    public Icon getThumbVerIcon() {
        return TextureIcons.getThumbVerIcon();
    }

    public Icon getThumbHorIconRollover() {
        return TextureIcons.getThumbHorIconRollover();
    }

    public Icon getThumbVerIconRollover() {
        return TextureIcons.getThumbVerIconRollover();
    }
}
