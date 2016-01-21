/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.wizard;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;

import java.util.*;

import static javax.swing.BoxLayout.*;

import vnmr.wizard.event.*;
import vnmr.wizard.navigator.*;

/**
 * Displays a wizard of panels; manages the back, next, finished functionality.
 *
 * Provides support for event handling through the WizardListener interface.
 * <br>
 * Example:
 * <pre>
 *       JWizard wizard = new JWizard( "Sample Wizard", new ImageIcon( "sample.jpg" ) );
 *       wizard.addWelcomePanel( "Welcome to the Sample Wizard",
 *                               "This wizard will guide you through setting up absolutely nothing\n\nEnjoy!" );
 *       wizard.addCustomPanel( "Dummy Panel 1", "Dummy1", new JPanel() );
 *       wizard.addCustomPanel( "Dummy Panel 2", "Dummy2", new JPanel() );
 *       wizard.addSummaryPanel( new String[] { "Copy files", "Update Environment" } );
 *       wizard.addFinishedPanel( "Thank you for installing MichaelSoft.\nYou can access your stuff through the things..." );
 *       wizard.addWizardListener( this );
 *       wizard.showWizard();
 * </pre>
 */
public class JWizard extends JDialog implements ActionListener {

    private static final long serialVersionUID = 1L;

    public static enum PageType {
        FIRST("To continue, click \"Next\"."),
        NORMAL("To continue, click \"Next\"."),
        LAST("To finish, click \"Finish\".");

        /** The label for the "Next" or "Finish" button */
        String nextButtonLabel;

        PageType(String label) {
            nextButtonLabel = label;
        }

        public String getNextText() {
            return nextButtonLabel;
        }
    }

    /**
     * Constant definition for the title font
     */
    public static final Font TITLE_FONT = new Font("Dialog", Font.BOLD, 18);

    /**
     * Constant definition for the text font
     */
    public static final Font TEXT_FONT = new Font( "Dialog", Font.BOLD, 14 );

    /**
     * The page logo; this is displayed on the left side of the wizard screen
     */
    //private ImageIcon logo;

    /**
     * The "Back" button
     */
    private JButton backButton = new JButton( "< Back" );

    /**
     * The "Next" button
     */
    private JButton nextButton = new JButton( "Next >" );

    /**
     * The "Cancel" button
     */
    private JButton cancelButton = new JButton( "Cancel" );

    /**
     * Content Panel that holds all of the wizard screens in a CardLayout
     */
    private JPanel m_contentPanel;

    private JPanel m_borderPanel = null;

    /**
     * Map of zero-based Integer indexes to wizard screen names
     */
    private Map<Integer, String> wizardScreens = new HashMap<Integer, String>();

    private ArrayList<JTextArea> m_messages = new ArrayList<JTextArea>();

    /**
     * Current screen index
     */
    private int currentScreen = 0;

    /**
     * Controls the flow from one screen to the next; to specify a custom
     * navigator, you must create a class that implements 
     * vnmr.wizard.navigator.Navigator and then set the following
     * System property: vnmr.wizard.navigator.class to reference
     * that class.
     * 
     * @see vnmr.wizard.navigator.Navigator
     */
    private Navigator navigator;

    /**
     * Wizard Listeners
     */
    private Set<WizardListener> listeners = new HashSet<WizardListener>();

    private JLabel m_currentIcon = null;

    /**
     * Creates a new JWizard with the specified title and application
     * logo
     */
    public JWizard( String title, Frame owner, Icon logo)
    {
        super(owner, true);
        // Set the title of the dialog box
        this.setTitle( title );

        // Set up the body panel
        m_borderPanel = new JPanel( new BorderLayout() );
        m_borderPanel.setBorder( BorderFactory.createEtchedBorder() );

        setIcon(logo);

        // Build the contentPanel
        m_contentPanel = new JPanel( new CardLayout() );
        m_borderPanel.add( m_contentPanel );
        this.getContentPane().add( m_borderPanel );

        // Build the button Panel
        JPanel buttonPanel = new JPanel(new FlowLayout(FlowLayout.RIGHT));
        buttonPanel.add( backButton );
        buttonPanel.add( nextButton );
        buttonPanel.add( cancelButton );
        backButton.addActionListener( this );
        nextButton.addActionListener( this );
        cancelButton.addActionListener( this );
        this.getContentPane().add( buttonPanel, BorderLayout.SOUTH );
        getRootPane().setDefaultButton(nextButton);

        // Set constraints
        //Dimension d = Toolkit.getDefaultToolkit().getScreenSize();
        //this.setSize(640, 480);/*CMP*/
        //this.setLocation( d.width / 2 - 320, d.height/2 - 240 );

        // Clicking on the close window is that same thing as cancelling
        // the wizard
        this.addWindowListener( new WindowAdapter() {
                public void windowClosing(WindowEvent e)
                {
                    setVisible( false );
                    fireWizardCancelled();
                }
                } );

        // Build our screen navigator 
        this.navigator = NavigatorFactory.getNavigator();
        this.navigator.init( this );
    }

    public void setNavigator(Navigator navigator) {
        this.navigator = navigator;
        this.navigator.init(this);
    }

    public void setIcon(Icon icon) {
        // Build the icon panel
        if (m_currentIcon != null) {
            m_borderPanel.remove(m_currentIcon);
        }
        JLabel iconLabel = new JLabel( icon );
        m_borderPanel.add( iconLabel, BorderLayout.WEST );
        m_currentIcon = iconLabel;
    }

    /**
     * Helper method that controls the state and text of the wizard
     * buttons
     */
    private void setPanelState()
    {
        if( this.currentScreen == 0 )
        {
            this.backButton.setEnabled( false );
            this.nextButton.setEnabled( true );
            this.nextButton.setText( "Next >" );
        }
        else if( this.currentScreen == this.wizardScreens.size() - 1 )
        {
            this.backButton.setEnabled( true );
            this.nextButton.setText( "Finish" );
        }
        else
        {
            //this.backButton.setEnabled( true );
            //this.nextButton.setEnabled( true );
            this.nextButton.setText( "Next >" );
        }
        this.nextButton.grabFocus();
    }

    public void setNextEnabled(boolean b) {
        this.nextButton.setEnabled(b);
    }

    public void setBackEnabled(boolean b) {
        this.backButton.setEnabled(b);
    }

    /**
     * Returns the map of zero-based Integer indexed keys to Screen Names
     */
    public Map<Integer, String> getWizardScreens()
    {
        return this.wizardScreens;
    }

    /**
     * Returns the index of the currently displayed screen
     */
    public int getCurrentScreenIndex()
    {
        return this.currentScreen;
    }

    /**
     * Returns the index of the currently displayed screen
     */
    public String getCurrentScreenName()
    {
        return getWizardScreens().get(getCurrentScreenIndex());
    }

    /**
     * Sets the index of the currently displayed screen
     */
    public void setCurrentScreenIndex( int currentScreen )
    {
        this.currentScreen = currentScreen;
    }


    /**
     * Event handling...
     */
    public void actionPerformed( ActionEvent ae )
    {
        // The user pressed back
        if( ae.getSource() == this.backButton ) {
            back();
        } else if( ae.getSource() == this.nextButton )
        {
            // The user pressed next
            //this.nextButton.grabFocus();
            if( this.currentScreen == this.wizardScreens.size() - 1 ) {
                // Last screen, we are done!
                finish();
            } else {
                // Advance to the next screen
                next();
            }
        } else if( ae.getSource() == this.cancelButton ) {
            // The user canceled the wizard
            cancel();
        }
    }

    public void back() {
        this.fireWizardScreenChanging("back");
        String currentName = this.wizardScreens.get( new Integer( this.currentScreen ) );
        String name = this.navigator.getNextScreen( currentName, Navigator.BACK );
        this.showPanel( name );
        this.fireWizardScreenChanged("back");
        // Update the buttons
        setPanelState();
    }

    public void next() {
        this.fireWizardScreenChanging("next");
        String currentName = this.wizardScreens.get(this.currentScreen);
        String name = this.navigator.getNextScreen( currentName, Navigator.NEXT );
        this.showPanel( name );
        this.fireWizardScreenChanged("next");
        // Update the buttons
        setPanelState();
    }

    public void finish() {
        this.setVisible( false );
        this.fireWizardComplete();
        // Update the buttons
        setPanelState();
    }

    public void cancel() {
        this.setVisible( false );
        this.fireWizardCancelled();
        // Update the buttons
        setPanelState();
    }

    /**
     * Returns a label that is consistent with the look-and-feel of
     * the Wizard
     * 
     * @param text          Text component of the label
     * @param f             The font of the label: TITLE_FONT, TEXT_FONT
     * @param alignment     The alignment of the text (from JLabel constants)
     */
    public JLabel getWizardLabel(String text, Font f, int alignment)
    {
        JLabel label = new JLabel(text, alignment);
        label.setFont(f);
        label.setBackground(null);
        label.setForeground(Color.black);
        label.setOpaque(true);
        return label;
    }

    /**
     * Returns a text area that is consistent with the look-and-feel of
     * the Wizard; it will be read only but look normal
     * 
     * @param text          Text component of the text area 
     */
    public JTextArea initializeWizardTextArea(String text)
    {
        JTextArea textArea = new JTextArea(text);
        textArea.setBackground(null);
        textArea.setFont(JWizard.TEXT_FONT);
        textArea.setEnabled(false);
        textArea.setDisabledTextColor(Color.black);
        textArea.setAlignmentX(JTextArea.CENTER_ALIGNMENT);
        return textArea;
    }

    public JPanel getBufferedBodyPanel(JComponent component)
    {
        JPanel panel = new JPanel(new BorderLayout());

        panel.setBorder(BorderFactory.createEmptyBorder(10, 10, 10, 10));
        panel.add(component, BorderLayout.CENTER);
        return panel;
    }

    public JPanel getBufferedBottomPanel( JComponent component )
    {
        JPanel panel = new JPanel( new BorderLayout() );

        panel.setBorder(BorderFactory.createEmptyBorder(10, 20, 10, 0));
        panel.add( component, BorderLayout.CENTER );
        return panel;
    }

    public JPanel getBufferedTopPanel( JComponent component )
    {
        JPanel panel = new JPanel( new BorderLayout() );

        panel.setBorder(BorderFactory.createEmptyBorder(10, 0, 0, 0));
        panel.add( component, BorderLayout.CENTER );
        return panel;
    }

    /**
     * Creates a Welcome Panel to display to the user
     * 
     * @param title     The title of the Wizard
     * @param message   The body of the welcome message
     */
    public void addWelcomePanel( String title,
                                 String message )
    {
        // Create the Panel
        JPanel panel = new JPanel( new BorderLayout() );
        panel.setBorder( BorderFactory.createEtchedBorder() );
        
        // Setup the fields
        panel.add( getBufferedTopPanel( getWizardLabel( title, JWizard.TITLE_FONT, JLabel.CENTER ) ), BorderLayout.NORTH );
        panel.add( getBufferedBodyPanel( initializeWizardTextArea( message ) ), BorderLayout.CENTER );
        JTextArea text = initializeWizardTextArea("To contine, click \"Next\".");
        m_messages.add(text);
        panel.add(getBufferedBottomPanel(text), BorderLayout.SOUTH);

        // Add the panel to the content panel
        addPanel( panel, "Welcome" );

        // Ensure that the panel is shown first
        showPanel( "Welcome" );
    }

    /**
     * Adds a summary panel of events that the system is about to
     * perform
     * 
     * @param steps     A list of all of the steps
     */
    public void addSummaryPanel( String[] steps )
    {
        // Create the Panel
        JPanel panel = new JPanel( new BorderLayout() );
        panel.setBorder( BorderFactory.createEtchedBorder() );
        
        // Setup the fields
        panel.add( getBufferedTopPanel( getWizardLabel( "Summary", JWizard.TITLE_FONT, JLabel.CENTER ) ), BorderLayout.NORTH );
        
        StringBuffer message = new StringBuffer( "The following steps will be performed:\n\n" );
        for( int i=0; i<steps.length; i++ )
        {
            message.append( "\t- " + steps[ i ] + "\n" );
        }

        panel.add( getBufferedBodyPanel( initializeWizardTextArea( message.toString() ) ), BorderLayout.CENTER );
        JTextArea text = initializeWizardTextArea("To proceed, click \"Next\".");
        m_messages.add(text);
        panel.add(getBufferedBottomPanel(text), BorderLayout.SOUTH);

        // Add the panel to the content panel
        addPanel( panel, "Summary" );
    }

    /**
     * Adds a Finished (or ThankYou) panel that provides information
     * after the wizard has completed
     * @param title TODO
     * @param message   The message to report to the user
     */
    public void addFinishedPanel(String title, String message)
    {
        // Create the Panel
        JPanel panel = new JPanel( new BorderLayout() );
        panel.setBorder( BorderFactory.createEtchedBorder() );
        
        // Setup the fields
        if (title == null) {
            title = "Finished";
        }
        panel.add(getBufferedTopPanel(getWizardLabel(title,
                                                     JWizard.TITLE_FONT,
                                                     JLabel.CENTER ) ),
                                                     BorderLayout.NORTH );
        panel.add(getBufferedBodyPanel(initializeWizardTextArea(message)),
                  BorderLayout.CENTER);
        JTextArea text = initializeWizardTextArea("To exit, click \"Finished\".");
        m_messages.add(text);
        panel.add(getBufferedBottomPanel(text), BorderLayout.SOUTH);

        // Add the panel to the content panel
        addPanel( panel, "Finished" );
    }

    /**
     * Adds a panel to the wizard
     * @param pageType FIRST, NORMAL, or LAST.
     */
    public void addCustomPanel(String title, String name, JPanel customPanel,
                               PageType pageType) {
        // Create the Panel
        JPanel panel = new JPanel( new BorderLayout() );
        //panel.setBorder( BorderFactory.createEtchedBorder() );
        
        // Setup the fields
        JLabel label = getWizardLabel(title, JWizard.TITLE_FONT, JLabel.CENTER);
        panel.add(getBufferedTopPanel(label), BorderLayout.NORTH );
        panel.add( getBufferedBodyPanel(customPanel), BorderLayout.CENTER );
        JTextArea text = initializeWizardTextArea(pageType.getNextText());
        m_messages.add(text);
        panel.add(getBufferedBottomPanel(text), BorderLayout.SOUTH);

        // Add the panel to the content panel
        addPanel( panel, name );
    }

    /**
     * Adds a panel to the wizard; in order of the panel to be shown
     * it must be added via this method
     */
    public void addPanel( JPanel panel, String name )
    {
        // Add the panel to the content panel
        this.m_contentPanel.add( panel, name );

        // Keep track of a the 0-based numerical index of each panel name
        int size = this.wizardScreens.size();
        this.wizardScreens.put( new Integer( size ), name );
    }

    /**
     * Displays panel with the specified name
     */
    public void showPanel( String name )
    {
        CardLayout cl = ( CardLayout )this.m_contentPanel.getLayout();
        cl.show( this.m_contentPanel, name );
        this.m_contentPanel.repaint();
    }

    /**
     * Displays the wizard
     */
    public void showWizard()
    {
        this.setPanelState();
        this.pack();
        this.setVisible( true );
    }

    /**
     * Adds a WizardListener to the JWizard that will be notified
     * when specific events occur
     */
    public void addWizardListener( WizardListener l )
    {
        this.listeners.add( l );
    }

    /**
     * Removes a WizardListener from the JWizard
     */
    public void removeWizardListener( WizardListener l )
    {
        this.listeners.remove( l );
    }


    protected void fireWizardCancelled()
    {
        WizardEvent we = new WizardEvent( this, "cancel" );
        for( Iterator<WizardListener> i=this.listeners.iterator(); i.hasNext(); )
        {
            WizardListener wl = i.next();
            wl.wizardCancelled( we );
        }
    }

    protected void fireWizardComplete()
    {
        WizardEvent we = new WizardEvent( this, "complete" );
        for( Iterator<WizardListener> i=this.listeners.iterator(); i.hasNext(); )
        {
            WizardListener wl = i.next();
            wl.wizardComplete( we );
        }
    }
    
    protected void fireWizardScreenChanged(String trigger)
    {
        WizardEvent we = new WizardEvent( this, trigger );
        for( Iterator<WizardListener> i=this.listeners.iterator(); i.hasNext(); )
        {
            WizardListener wl = i.next();
            wl.wizardScreenChanged( we );
        }
    }

    protected void fireWizardScreenChanging(String trigger)
    {
        WizardEvent we = new WizardEvent( this, trigger );
        for( Iterator<WizardListener> i=this.listeners.iterator(); i.hasNext(); )
        {
            WizardListener wl = i.next();
            wl.wizardScreenChanging( we );
        }
    }

    public static JPanel getWizardPanel(Color bkg) {
        JPanel panel = new JPanel();
        panel.setBackground(bkg);
        panel.setLayout(new BoxLayout(panel, Y_AXIS));
        return panel;
    }

    public void setMessage(String string) {
        int n = getCurrentScreenIndex();
        //JPanel panel = (JPanel)this.contentPanel.getComponent(n);
        m_messages.get(n).setText(string);
    }

}
