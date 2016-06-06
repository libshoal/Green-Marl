from pygments.lexer import RegexLexer, bygroups
from pygments.token import *

__all__ = [ 'GMLexer' ]

# Put this in pygments/lexters
#
# make mapfiles - rebuild the list of lexers
#
# Then, render a Green Marl program, e.g.
# ./pygmentize -O full -f html -o /tmp/pagerank.html pagerank.gm

class GMLexer(RegexLexer):
    name = 'Green Marl'
    aliases = ['gm']
    filenames = ['*.gm']

    keywords = ['While', 'Foreach', 'Sum', 'Do ',
                'Procedure', '\.Nodes', '\.InNbrs', 'Node_Prop']
    operators = ['\&\&', '<', '>', '\+=', '/', '@', '\+\+', '\+',
                 '-', '\*', ',', '\.', '\|' ]
    types = ['Double', 'Int', 'Graph']
    punctuation = [ '{', '}', '\[', '\]', '\(', '\)', ';', ':', '=',  ]
    buildin = ['OutDegree', 'NumNodes']
    
    tokens = {
        'root': [
            (r'\s+', Text),
            (r'[0-9]+.[0-9]+', Number.Float),
            (r'[0-9]+', Number.Integer),
        ] +
        [ ('%s\(\)' % s, Name.Function) for s in buildin ] +
        [ ('%s' % s, Keyword) for s in keywords ] +
        [ ('%s' % s, Keyword.Type) for s in types ] +
        [ ('%s' % s, Operator) for s in operators ] +
        [ ('%s' % s, Punctuation) for s in punctuation ] +
        [(r'[a-zA-Z_]+', Text)]
    }
