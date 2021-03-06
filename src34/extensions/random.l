import system'routines.

// --- RandomGenerator ---

sealed struct RandomGenerator
{
    long theSeed.
    
    constructor new
    [
        system'external core_rnd_init(&theSeed).
    ]

    eval(int aMaxValue, ref<int> aRetVal)
    [
        system'external core_rnd_next(theSeed, aMaxValue, &aRetVal).
    ]

    eval(ref<int> aRetVal)
    [
        system'external core_rnd_nextint(theSeed, &aRetVal).
    ]

    eval(object minValue, object maxValue)
        = minValue + self eval(maxValue - minValue).

    eval : aMaxValue
    [
        int aNextValue := 0.
        self eval(T<int>(aMaxValue), &aNextValue).
        
        ^ IntNumber new(aNextValue).
    ]
    
    nextInt
    [
        int aNextValue := 0.
        self eval(&aNextValue).
        
        ^ IntNumber new(aNextValue).
    ]

    nextInt : aValue
        = self eval:aValue.
        
    real nextReal
    [
        int next := 0.
        self eval(1000000, &next).
        
        real retVal := 0.0r.
        realConvertor convert(next, &retVal).
        
        retVal := retVal / 1000000.0r.
        
        ^ RealNumber new(retVal).
    ]
}

// --- randomGenerator ---

public static randomGenerator = RandomGenerator new.

// -- ERandomizer --

public extension randomOp
{
    randomize : aCount
    [
        var list := self clone.
        
        int n := T<int>(aCount).
        int i := 0.
        
        var max := self length.
        
        while (i < n)
        [
            var j := randomGenerator eval:max.
            
            list~indexableOp exchange(i, j).
            
            i := i + 1
        ].
        
        ^ list
    ]
    
    
}
