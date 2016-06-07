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

    keywords = ['While', 'Foreach', 'Sum', 'Do ', 'Exist', 'Return',
                'Procedure', '\.Nodes', '\.InNbrs', 'Node_Prop', '\.Nbrs' ]
    operators = ['\&\&', '<', '>', '\+=', '/', '@', '\+\+', '\+',
                 '-', '\*', ',', '\.', '\|', '!', '\?' ]
    types = ['Double', 'Int', 'Graph', 'Bool', 'Long']
    punctuation = [ '{', '}', '\[', '\]', '\(', '\)', ';', ':', '=',  ]
    buildin = ['OutDegree', 'NumNodes', 'HasEdgeTo' ]
    constants = [ '\+INF', 'True', 'False' ]
    
    tokens = {
        'root': \
        [ ('//.*', Comment) ] +
        [ ('%s' % s, Number.Constant ) for s in constants ] +
        [
            (r'\s+', Text),
            (r'[0-9]+.[0-9]+', Number.Float),
            (r'[0-9]+', Number.Integer),
            (r'\+INF', Number.Integer),
        ] +
        [ ('%s\(\)' % s, Name.Function) for s in buildin ] +
        [ ('%s\(' % s, Name.Function) for s in buildin ] +
        [ ('%s' % s, Keyword) for s in keywords ] +
        [ ('%s' % s, Keyword.Type) for s in types ] +
        [ ('%s' % s, Operator) for s in operators ] +
        [ ('%s' % s, Punctuation) for s in punctuation ] +
        [(r'[a-zA-Z_]+', Text)]
    }
