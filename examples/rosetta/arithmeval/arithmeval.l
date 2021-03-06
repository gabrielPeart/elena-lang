/// a program which parses and evaluates arithmetic expressions. 

import system'routines.
import extensions.
import extensions'text.

// --- Token ---

class Token
{
    object theValue.
    
    int rprop level :: theLevel.
    
    constructor new(int aLevel)
    [
        theValue := StringWriter new.
        theLevel := aLevel + 9.
    ]
    
    append : aChar
    [
        theValue << aChar.
    ]
    
    number = theValue toReal.
}

// --- Node ---

class Node
{
    object prop  left  :: theLeft.
    object prop  right :: theRight.
    int    rprop level :: theLevel.

    constructor new(int aLevel)
    [
        theLevel := aLevel.
    ]
}

// --- SummaryNode

class SummaryNode :: Node
{
    constructor new(int aLevel)
        <= new(aLevel + 1).
    
    number = theLeft number + theRight number.
}

// --- DifferenceNode ---

class DifferenceNode :: Node
{
    constructor new(int aLevel)
        <= new(aLevel + 1).
    
    number = theLeft number - theRight number.
}

// --- ProductNode ---

class ProductNode :: Node
{
    constructor new(int aLevel)
        <= new(aLevel + 2).
    
    number = theLeft number * theRight number.
}

// --- FractionNode ---

class FractionNode :: Node
{
    constructor new(int aLevel)
        <= new(aLevel + 2).
    
    number = theLeft number / theRight number.
}

// --- Expression ---

class Expression
{
    int    rprop level :: theLevel.
    object prop  top   :: theTop.
    
    constructor new(int aLevel)
    [
        theLevel := aLevel
    ]
    
    right = theTop.

    set right:aNode [ theTop := aNode ]
    
    number => theTop.
}

// --- States ---

singleton operatorState
{
    eval(ch)
    [
        ch =>
            $40 [      // (
                ^ target newBracket; gotoStarting
            ];
            ! [
                ^ target newToken; append:ch; gotoToken
            ].
    ]
}

singleton tokenState
{
    eval(ch)
    [
        ch =>
            $41 [      // )
                ^ target closeBracket; gotoToken
            ];
            $42 [      // *
                ^ target newProduct; gotoOperator
            ];
            $43 [      // +
                ^ target newSummary; gotoOperator
            ];
            $45 [      // -
                ^ target newDifference; gotoOperator
            ];
            $47 [      // /
                ^ target newFraction; gotoOperator
            ];
            ! [
                ^ target append:ch
            ].
    ]
}    

singleton startState
{
    eval(ch)
    [
        ch =>
            $40 [      // (
                ^ target newBracket; gotoStarting
            ];
            $45 [ // -
                ^ target newToken; append:"0"; newDifference; gotoOperator
            ];
            ! [
                ^ target newToken; append:ch; gotoToken
            ].
    ]
}    

// --- Scope ---

class Scope
{
    object theState.
    int    theLevel.
    object theParser.
    object theToken.
    object theExpression.
    
    constructor new:aParser
    [
        theState := startState.
        theLevel := 0.
        theExpression := Expression new(0).
        theParser := aParser.
    ]
                                       
    newToken
    [
        theToken := theParser appendToken(theExpression, theLevel).
    ]                                                
    
    newSummary
    [
        theToken := nil.
        
        theParser appendSummary(theExpression, theLevel).
    ]                                                
    
    newDifference
    [
        theToken := nil.
        
        theParser appendDifference(theExpression, theLevel)
    ]                                                
      
    newProduct
    [
        theToken := nil.
        
        theParser appendProduct(theExpression, theLevel)
    ]                                                
    
    newFraction
    [
        theToken := nil.
        
        theParser appendFraction(theExpression, theLevel)
    ]                                                

    newBracket
    [
        theToken := nil.
        
        theLevel := theLevel + 10.
        
        theParser appendSubexpression(theExpression, theLevel).
    ]            

    closeBracket
    [
        if (theLevel < 10)
            [ InvalidArgumentException new:"Invalid expression"; raise ].
        
        theLevel := theLevel - 10
    ]            
                                    
    append:ch
    [
        if((ch >= $48) && (ch < $58))
            [ theToken append:ch ];
            [ InvalidArgumentException new:"Invalid expression"; raise ]
    ]
            
    append(literal aLiteral)
    [
        aLiteral forEach(:ch)[ self append:ch ]
    ]
                                                         
    gotoStarting
    [
        theState := startState
    ]
                                                                  
    gotoToken
    [
        theState := tokenState
    ]
                                                                  
    gotoOperator
    [
        theState := operatorState
    ]
    
    number => theExpression.
    
    dispatch => theState.
}

// ---- Parser ----

class Parser
{
    appendToken(object anExpression, int aLevel)
    [
        var aToken := Token new(aLevel).
        
        anExpression top := self append(anExpression top, aToken).
        
        ^ aToken
    ]

    appendSummary(object anExpression, int aLevel)
    [
        anExpression top := self append(anExpression top, SummaryNode new(aLevel)).
    ]

    appendDifference(object anExpression, int aLevel)
    [
        anExpression top := self append(anExpression top, DifferenceNode new(aLevel)).
    ]

    appendProduct(object anExpression, int aLevel)
    [
        anExpression top := self append(anExpression top, ProductNode new(aLevel)).
    ]

    appendFraction(object anExpression, int aLevel)
    [
        anExpression top := self append(anExpression top, FractionNode new(aLevel))
    ]

    appendSubexpression(object anExpression, int aLevel)
    [
        anExpression top := self append(anExpression top, Expression new(aLevel)).
    ]

    append(object aLastNode, object aNewNode)
    [
        if(nil == aLastNode)
            [ ^ aNewNode ].
        
        if (aNewNode level <= aLastNode level)
            [ aNewNode left := aLastNode. ^ aNewNode ].
            
        var aParent := aLastNode.
        var aCurrent := aLastNode right.
        while ((nil != aCurrent) && (aNewNode level > aCurrent level))
            [ aParent := aCurrent. aCurrent := aCurrent right. ].
        
        if (nil == aCurrent)
            [ aParent right := aNewNode. ];
            [ aNewNode left := aCurrent. aParent right := aNewNode ].
            
        ^ aLastNode
    ]
    
    run : aText
    [
        var aScope := Scope new(self).

        aText forEach(:ch)[ aScope eval:ch ].

        ^ aScope number
    ]
}

public program
[
    var aText := StringWriter new.
    var aParser := Parser new.

    $(console readLine; saveTo:aText; length > 0) doWhile:
    [
        try(console printLine("=",aParser run:aText))
        {
            on(Exception e)
            [
                console writeLine:"Invalid Expression"
            ]
        }.
                
        aText clear
    ]
]