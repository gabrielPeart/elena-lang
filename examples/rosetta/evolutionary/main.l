import system'routines.
import extensions.
import extensions'text.

const literal Target = "METHINKS IT IS LIKE A WEASEL".
const literal AllowedCharacters = " ABCDEFGHIJKLMNOPQRSTUVWXYZ".

const int C = 100.
const real P = 0.05r.

rnd = randomGenerator.

randomChar 
    = AllowedCharacters[rnd nextInt(AllowedCharacters length)].

extension evoHelper
{
    randomString
        = 0 till:self repeat(:x)( randomChar ); summarize(StringWriter new); literal.
        
    fitnessOf:s
        = self zip:s by(:a:b)( (a == b)iif(1,0) ); summarize(Integer new); toInt.
        
    mutate : p
        = self selectBy(:ch)( (rnd nextReal <= p) iif(randomChar,ch) ); summarize(StringWriter new); literal.
}

class EvoAlgorithm :: Enumerator
{
    object theTarget.
    object theCurrent.
    object theVariantCount.

    constructor new : s of:count
    [
        theTarget := s.
        theVariantCount := count toInt.
    ]

    get = theCurrent.

    bool next
    [
        if (nil == theCurrent)
            [ theCurrent := theTarget length; randomString. ^ true ].
            
        if (theTarget == theCurrent)
            [ ^ false ].
            
        auto variants := Array new:theVariantCount; populate(:x)( theCurrent mutate:P ).
        
        theCurrent := variants sort(:a:b)( a fitnessOf:Target > b fitnessOf:Target ); getAt:0.
        
        ^ true.
    ] 
    
    reset
    [
        theCurrent := nil.
    ]
    
    enumerable => theTarget.                          
}

public program
[
    var attempt := Integer new.
    EvoAlgorithm new:Target of:C; forEach(:current)
    [
        console 
            printPaddingLeft(10,"#",attempt append:1);
            printLine(" ",current," fitness: ",current fitnessOf:Target).
    ].
    
    console readChar
]