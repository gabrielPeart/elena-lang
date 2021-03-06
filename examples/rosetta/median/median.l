import system'routines.
import system'math.
import extensions.
 
extension op 
{
    median
    [
        var aSorted := self ascendant.

        var aLen := aSorted length.
        if (aLen == 0)
            [ ^ nil ];
            [
                var aMiddleIndex := aLen / 2.
                if (aLen mod:2 == 0)
                    [ ^ (aSorted[aMiddleIndex - 1] + aSorted[aMiddleIndex]) / 2 ];
                    [ ^ aSorted[aMiddleIndex] ]
            ]
    ]
}    

public program
[
    var a1 := (4.1r, 5.6r, 7.2r, 1.7r, 9.3r, 4.4r, 3.2r).
    var a2 := (4.1r, 7.2r, 1.7r, 9.3r, 4.4r, 3.2r).
    
    console printLine("median of (",a1,") is ",a1 median).
    console printLine("median of (",a2,") is ",a2 median).
    
    console readChar
]