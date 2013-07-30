# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

# Initial version by Matthias Baas (baas@ira.uka.de).

"""defines a class that helps to format user messages"""

import os, logging, textwrap

class multi_line_formatter_t(logging.Formatter):
    """Custom log formatter to split long message into several lines.

    This formatter is used for the default stream handler that outputs
    its messages to `stdout`.
    """

    def __init__(self, fmt=None, datefmt=None, width=None):
        """Constructor.

        See the Python standard library reference for a documentation
        of fmt and datefmt.
        width is the maximum width of the generated text blocks.
        """
        logging.Formatter.__init__(self, fmt, datefmt)
        if None == width:
            try:
                import curses
                curses.setupterm()
                self._width = curses.tigetnum('cols')
                # Leave a border of two blanks to prevent that full lines
                # wrap to the next line
                self._width -= 2
            except:
                self._width = 70   # default to 70

    def format(self, record):
        """This method overwrites the original one.

        The first thing, that is done in the original format() method, is the
        creation of the record.message attribute:

          record.message = record.getMessage()

        Now this method temporarily replaces the getMessage() method of
        the record by a version that returns a generated message that
        spans several lines. Then the original format() method is called
        which will invoke the 'fake' method.
        """
        # Get the original single-line message
        message = record.getMessage()
        # Distribute the message among several lines...
        message = self.formatMessage(message, width=self._width)
        # ...and temporarily replace the getMessage() method so that the
        # reformatted message is used
        mth = record.getMessage
        record.getMessage = lambda x=message: x
        # Invoke the inherited format() method
        res = logging.Formatter.format(self, record)
        # Restore the original method
        record.getMessage = mth
        return res

    @staticmethod
    def formatMessage(msgline, width=70):
        """Format a long single line message so that it is easier to read.

        `msgline` is a string containing a single message. It can either be
        a plain message string which is reformatted using the :mod:`textwrap`
        module or it can be of the form <declaration>;<message> where <declaration>
        is the declaration string and <message> an arbitrary message. Lines of this
        form will be separated so that the declaration and the message appear in
        individual text blocks, where every line of message will start
        with '>' character.

        width is the maximum width of any text blocks (without indentation).
        """
        if isinstance(msgline, logging.LogRecord):
            msgline = msgline.getMessage()
        txts = msgline.split(";")
        # Ensure that there are no more than two items in txts
        if len( txts ) != 2:
            #If message is not in format we expected, just return it
            return os.linesep.join( textwrap.wrap( msgline, width ) )

        lines = [ txts[0] ] #I don't want to break declaration string to few lines

        # Insert a separator if there are two parts (=decl and msg)
        # Apply the text wrapper to shorten the maximum line length
        wrapped_lines = textwrap.wrap( txts[1], width )
        lines.extend( ["> " + s.strip() for s in wrapped_lines] )

        return os.linesep.join(lines)

