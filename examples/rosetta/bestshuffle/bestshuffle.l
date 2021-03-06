/// Shuffles the characters of a string in such a way that as many of the character values are in a different position as possible.

import system'routines.
import extensions.
import extensions'text.

extension op
{
    shuffled
    [
        var anOriginal := self toArray.
        var aShuffled := self toArray.
    
        0 to:(anOriginal length - 1) do(:i)
        [
            0 to:(anOriginal length - 1) do(:j)
            [
                if ((i != j) && (anOriginal[i] != aShuffled[j]) && $(anOriginal[j] != aShuffled[i]))
                [
                    aShuffled exchange(i,j)
                ].
            ].
        ].
    
        ^ aShuffled summarize(StringWriter new); toLiteral
    ]
    
    score : anOriginalText
    [
        var aShuffled := self toArray.
        var anOriginal := anOriginalText toArray.
        var aScore := Integer new.

        0 to(anOriginal length - 1) do(:i)
            [ if (anOriginal[i] == aShuffled[i]) [ aScore append:1 ] ].
        
        ^ aScore value.
    ]
}

// --- Program ---

public program
[
    ("abracadabra", "seesaw", "grrrrrr", "pop", "up", "a") forEach(:s)
    [
        var shuffled_s := s shuffled.

        console printLine("The best shuffle of ",s," is ",shuffled_s,"(",shuffled_s score:s,")").
    ].

    console readChar
]