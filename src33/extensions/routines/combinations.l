import system'routines.
import extensions.

type $strategy = BaseStrategy.

// --- Strategy ---

limited class BaseStrategy
{
    getAt array:Indexers int:aLength = $nil.
    
    $strategy eval array:Indexers int:aLength = $self.
    
    bool result = false.
}

// --- Combination strategies ---

class combinationEnd = BaseStrategy::
{
    bool result = false.
}.

class sequentialNext = BaseStrategy::
{
    bool result = true.
    
    $strategy eval array:Indexers int:aLength
    [
        if($nil == (Indexers seekEach(:anIndexer)
                [
                    indexer it := anIndexer indexer. 
                    
                    ifnot(it append index:1; available)
                        [ it writeIndex int:0. ^ false ].
                        
                    ^ true
                ]))
            [ ^ combinationEnd ].
            
        ^ $self          
    ]
        
    getAt array:Indexers int:aLength
    [
        ^ Array new int:aLength; populate(:i)<int>(Indexers getAt int:i; get)
    ]
}.

class sequentialStart = BaseStrategy::
{
    $strategy eval array:Indexers int:aLength
    [
        Indexers forEach(:anIndexer) [ anIndexer writeIndex int:0 ].
        
        ^ sequentialNext
    ]
}.

// --- Parallel strategies

class parallelNext = BaseStrategy::
{
    bool result = true.
    
    $strategy eval array:Indexers int:aLength
    [
        if($nil != Indexers seekEach(:anIndexer)(anIndexer append index:1; available; not ))
            [ ^ combinationEnd ].
            
        ^ $self       
    ]
        
    getAt array:Indexers int:aLength
    [
        ^ Array new int:aLength; populate(:i)<int>(Indexers array; getAt int:i; get ).
    ]
}.

class parallelStart = BaseStrategy::
{
    $strategy eval array:Indexers int:aLength
    [
        Indexers forEach(:anIndexer)[ anIndexer writeIndex int:0 ].
        
        ^ parallelNext
    ]
}.

// --- combination strategy ---

class combinationNext = BaseStrategy::
{
    bool result = true.

    getAt array:Indexers int:aLength
    [
        ^ Array new int:aLength; populate(:i)<int>(Indexers array; getAt int:i; get)
    ]
        
    $strategy eval array:Indexers int:aLength
    [
        array anArray := Indexers array.
        int   k := anArray length.
        int   i := k - 1.
        
        indexer aCurrentIndexer := anArray getAt int:i; indexer.
        
        if(aCurrentIndexer append index:1; available)
            [ ^ $self ].
             
        while ((i > 0) && (anArray getAt int:(i - 1); indexer; index; int + k - i + 1 >= aLength))
            [ i := i - 1 ].
        
        if(i == 0)
            [ ^ combinationEnd ].
        
        anArray getAt int:(i - 1); append index:1.
        
        //i to &int:(k - 1)
        IntNumber new int:i; runIntLoop int:(k - 1)
            nfunc: (:j)<int>[ anArray getAt int:j; writeIndex int:(IntNumber new int:(anArray getAt int:(j - 1); index; int + 1)) ].
            
        ^ $self          
    ]
}.

class combinationStart = BaseStrategy::
{
    $strategy eval array:Indexers int:n
    [
        array anArray := Indexers array.
        int aLength := anArray length.
        
        0~intLoopOp to(aLength - 1) do(:i)<int>[ anArray getAt int:i; indexer; writeIndex int:i ].
        
        ^ combinationNext
    ]
}.

// --- BaseRowEnumerator ---

class BaseRowEnumerator :: Enumerator
{
    array theIndexers.
    int   theLength.
    
    $strategy theStrategy.

    constructor new(object<> Arguments)
    [
        theLength := Arguments length.
        theIndexers := Array new int:theLength.
        
        int i := 0.
        while (i < theLength)
        [
            theIndexers setAt int:i object(Arguments[i] indexer).
            
            i := i + 1
        ].
        
        $self reset.
    ]
    
    constructor new : Arguments
    [
        theLength := Arguments length.
        theIndexers := Array new int:theLength.
        
        int i := 0.
        while (i < theLength)
        [
            theIndexers setAt int:i object(Arguments[IntNumber new int:i] indexer).
            
            i := i + 1
        ].
        
        $self reset
    ]
    
    bool next
    [
        theStrategy := theStrategy eval array:theIndexers int:theLength.

        ^ theStrategy result
    ]
    
    get
        = theStrategy getAt array:theIndexers int:theLength.
}

// --- SequentialEnumerator ---

class SequentialEnumerator :: BaseRowEnumerator
{
    constructor new : Arguments
        <= new:Arguments.
    
    constructor new(object<> Arguments)
        <= new object<>:Arguments.

    reset
    [
        theStrategy := sequentialStart.
    ]
}

// --- ParallelEnumerator ---

class ParallelEnumerator :: BaseRowEnumerator
{
    constructor new : Arguments
        <= new:Arguments.
    
    reset
    [
        theStrategy := parallelStart.
    ]
}

// --- Combinator ---

class Combinator :: BaseRowEnumerator
{
    int theTotalLength.
    
    constructor new : aSubLength of:aSet
        <= new : (Array new int:(aSubLength int); populate(:n)<int>(aSet));
    [
        theTotalLength := aSet length.
    ]
    
    bool next
    [
        theStrategy := theStrategy eval array:theIndexers int:theTotalLength.

        ^ theStrategy result
    ]
    
    reset
    [
        theStrategy := combinationStart
    ]
}
