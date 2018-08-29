/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.util.NoSuchElementException;
import java.util.StringTokenizer;


/**
 * Similar to the standard StringTokenizer, but returns quoted strings
 * as a single token.  Quote characters may be part of a quoted string
 * if they are preceded with the escape character ("\").  In this case
 * the escape character is removed in the returned token.
 * <p>
 * Note: Unescaped quotes embedded in an unquoted string are OK
 * if the string does not start with a quote and contains no spaces.
 * In this case, any escape character is not removed from the
 * returned token.
 * <p>
 * Restrictions: Only one character can be specified as a quote character.
 */
public class QuotedStringTokenizer extends StringTokenizer {
    static private final String DEFAULT_QUOTE = "\"";
    static private final String DEFAULT_DELIMS = " \t\n\r\f";

    private String delims;
    private StringBuffer sbuf;
    private int maxDelim;
    private String sQuote;
    private char cQuote;
    private boolean groupAdjacentDelims = true;

    /**
     * Constructs a string tokenizer for the specified string. The
     * tokenizer uses the default delimiter set, which is " \t\n\r\f":
     * the space character, the tab character, the newline character,
     * the carriage-return character, and the form-feed character.
     * Delimiter characters themselves will not be treated as tokens.
     * @param str A string to be parsed.
     */
    public QuotedStringTokenizer(String str) {
        this(str, DEFAULT_DELIMS, DEFAULT_QUOTE);
    }

    /**
     * Constructs a string tokenizer for the specified string, with
     * the specified delimiters. The characters in the delim argument
     * are the delimiters for separating tokens. Delimiter characters
     * themselves will not be treated as tokens.
     * @param str A string to be parsed.
     * @param delims The delimiters.
     */
    public QuotedStringTokenizer(String str, String delims) {
        this(str, delims, DEFAULT_QUOTE);
    }
    
    /**
     * Constructs a string tokenizer for the specified string, with
     * the specified delimiters. The characters in the delim argument
     * are the delimiters for separating tokens. Delimiter characters
     * themselves will not be treated as tokens.  adjacent (groupAdjacentDelims)
     * specifies whether or not adjacent delimineters will give empty tokens
     * or be treated at a single deliminiter.  That is if you are using
     * "space" as the deliminiter, you want to allow groups of spaces to be
     * a single deliminiter.  However, if you are using "," you may want
     * two adjacent "," to give an empty token.
     * @param str A string to be parsed.
     * @param delims The delimiters.
     */
    public QuotedStringTokenizer(String str, String delims, boolean adjacent) {
        this(str, delims, DEFAULT_QUOTE);
        groupAdjacentDelims = adjacent;
    }
    

    /**
     * Constructs a string tokenizer for the specified string, with
     * the specified delimiters and quote character. The characters in
     * the delim argument are the delimiters for separating
     * tokens. Delimiter characters themselves will not be treated as
     * tokens.
     * <p>
     * Note: Only one quote character can be specified.
     * @param str A string to be parsed.
     * @param delims The delimiters.
     * @param quote A string of length one (1) giving the quote character.
     */
    public QuotedStringTokenizer(String str, String delims, String quote) {
        super(str, delims);
        if (quote == null || quote.length() != 1) {
            quote = "\"";
        }
        this.sQuote = quote;
        this.cQuote = quote.charAt(0);
        this.delims = delims;
        sbuf = new StringBuffer(str);
        setMaxDelimChar();
    }

    private void setMaxDelimChar() {
        if (delims == null || delims.length() == 0) {
            maxDelim = -1;
            return;
        }

        char m = 0;
        for (int i = 0; i < delims.length(); i++) {
            char c = delims.charAt(i);
            if (m < c)
                m = c;
        }
        maxDelim = m;
    }

    /**
     * Tests if there are more tokens available from this tokenizer's
     * string. If this method returns true, then a subsequent call to
     * nextToken with no argument will successfully return a token.
     */
    public boolean hasMoreTokens() {
        int len = sbuf.length();
        if(!groupAdjacentDelims && len > 0) {
            char c = sbuf.charAt(0);
            if(delims.indexOf(c) >= 0)
                return true;
        }
        for (int i = 0; i < len; ++i) {
            char c = sbuf.charAt(i);
            if (c > maxDelim || delims.indexOf(c) < 0) {
                return true;
            }
        }
        return false;
    }

    /**
     * Returns the next token from this string tokenizer.
     */
    public String nextToken() throws NoSuchElementException {
        String token;

        if (sbuf.length() == 0) {
            throw new NoSuchElementException();
        }
        // Skip all leading delimiters
        if (groupAdjacentDelims) {
            while (sbuf.length() > 0) {
                char ch = sbuf.charAt(0);
                if (ch <= maxDelim && delims.indexOf(ch) >= 0) { // is ch a
                                                                    // delim?
                    sbuf.deleteCharAt(0);
                }
                else {
                    break;
                }
            }
        }
        // Skip one leading delimiters
        else {
            char ch = sbuf.charAt(0);
            if (ch <= maxDelim && delims.indexOf(ch) >= 0) { // is ch a
                                                                // delim?
                sbuf.deleteCharAt(0);
            }
        }
        
        // Allow empty string after last delimiter to return a valid empty string
        if (sbuf.length() == 0)
            return "";
        
        if (sbuf.charAt(0) == cQuote) {
            // Find closing quote
            int end = sbuf.indexOf(sQuote, 1);
            while (end > 0) {
                // Ignore escaped quotes (but delete the escapes)
                if (sbuf.charAt(end - 1) != '\\') {
                    break;
                } else {
                    sbuf.deleteCharAt(end - 1);
                    // NB: Since "\" is deleted, "end" is one past sQuote now
                    end = sbuf.indexOf(sQuote, end);
                }                    
            }
            if (end < 0) {
                token = sbuf.substring(1);
                sbuf.delete(0, sbuf.length());
            } else {
                token = sbuf.substring(1, end);
                sbuf.delete(0, end + 1);
            }
        } else {
            // Find end of non-delimiters
            int end;
            int limit = sbuf.length();
            char ch = sbuf.charAt(0);
            // is ch a delim?
            if (!groupAdjacentDelims && ch <= maxDelim
                    && delims.indexOf(ch) >= 0) {
                // If the first char is a deliminater, and we are not
                // grouping deliminaters, return an empty string
                end = 0;
            }
            else {
                for (end = 1; end < limit; ++end) {
                    char c = sbuf.charAt(end);
                    if (c <= maxDelim && delims.indexOf(c) >= 0) {
                        break;
                    }
                }
            }
            token = sbuf.substring(0, end);
            sbuf.delete(0, end);
        }
        return token;
    }

    /**
     * Returns the next token from this string tokenizer.
     * First, the set of characters considered to be delimiters by
     * this QuotedStringTokenizer object is changed to be the
     * characters in the string delim. Then the next token in the
     * string after the current position is returned. The current
     * position is advanced beyond the recognized token. The new
     * delimiter set remains the default after this call.
     */
    public String nextToken(String delim) {
        this.delims = delim;
        setMaxDelimChar();
        return nextToken();
    }

    /**
     * Returns the same value as the nextToken method, except that its
     * declared return value is Object rather than String.
     */
    public Object nextElement() {
        // This is actually the same definition as in StringTokenizer,
        // but we don't rely on the StringTokenizer implementation.
        return nextToken();
    }

    /**
     * Calculates the number of times that this tokenizer's nextToken
     * method can be called before it generates an exception. The
     * current position is not advanced.
     */
    public int countTokens() {
        // TODO: Should maybe go for a faster implementation.
        // (Cache the tokens that we get?)
        QuotedStringTokenizer qtok
                = new QuotedStringTokenizer(remainingString(), delims, sQuote);
        int count = 0;
        try {
            while (true) {
                qtok.nextToken();
                count++;
            }
        } catch (NoSuchElementException nsee) {
        }
        return count;
    }

    /**
     * Returns the remainder of the string (that has not yet been
     * tokenized).  This does not change the state of the tokenizer;
     * token parsing can continure after this call.
     */
    public String remainingString() {
        return sbuf.toString();
    }
}
