import system'routines;
import system'text;

// --- outputOp ---

public extension outputOp
{
    print(params object[] list)
    {
        int len := list.Length;
        for(int i := 0, i < len, i := i + 1)
        {
            object obj := list[i];
            
            self.write(obj.toString())
        }
    }
//    
////    generic printWide(object<> list)
////    [
////        int len := list length.
////        int i := 0.
////        while (i < len)
////        [
////            object obj := list[i].
////            
////            self write(obj~convertorOp toWide).
////            
////            i := i + 1
////        ].
////        
////        ^ self
////    ]
//
//    printLine
//    [
//        self writeLine.
//    ]
            
    printLine(params object[] list)
    {
        int len := list.Length;
        for(int i := 0, i < len, i := i + 1)
        {
            object obj := list[i];
            
            self.write(obj.toString())
        };
        
        
        self.writeLine();
    }    
}

public extension outputFormatterOp
{
    printPaddingRight(int width, params object[] list)
    {
        auto buffer := new TextBuilder();
        
        int len := list.Length;
        for (int i := 0, i < len, i += 1)
        {
            object obj := list[i];
            
            buffer.write(obj.toString())
        };
        
        self.writePaddingRight(buffer.Value, $32, width);
            
        ^ self
    }

    printPaddingLeft(int width, params object[] list)
    {
        auto buffer := new TextBuilder();
        
        int len := list.Length;
        for (int i := 0, i < len, i += 1)
        {
            object obj := list[i];
            
            buffer.write(obj.toString())
        };
        
        self.writePaddingLeft(buffer.Value, $32, width);
            
        ^ self
    }
}

//public extension outputCharFormatterOp
//{
//    generic printPaddingRightChar(int width, char ch, V<object> list)
//    [
//        auto aBuffer := TextBuilder new.
//        
//        int len := list length.
//
//        int i := 0.
//        while (i < len)
//        [
//            object obj := list[i].
//            
//            aBuffer write(obj~convertorOp toLiteral).
//            
//            i := i + 1
//        ].
//        
//        self write:(aBuffer literal) paddingRight:width with:ch.
//            
//        ^ self
//    ]
//
//    generic printPaddingLeftChar(int width, char ch, V<object> list)
//    [
//        auto aBuffer := TextBuilder new.
//        
//        int len := list length.
//        int i := 0.
//        while (i < len)
//        [
//            object obj := list[i].
//            
//            aBuffer write(obj~convertorOp toLiteral).
//            
//            i := i + 1
//        ].
//        
//        self write:(aBuffer literal) paddingLeft:width with:ch.
//            
//        ^ self
//    ]    
//}
//
//// --- formatterOp ---
//
//singleton helper
//{
//    readIndex(literal s, ref<int> i, ref<int> retVal)
//    [
//        char ch := $0.
//        int  n := 0.
//        int  v := 0.
//        
//        s read(T<int>(i), &ch).
//        while (ch != $125)
//        [
//            intConvertor convert(ch, &n).
//            if((n >= 30h)&&(n <= 39h))
//            [                
//                n := n - 30h.
//                
//                v := v * 10.
//                v := v + n.
//            ];
//            [ InvalidArgumentException new; raise ].
//                
//            i append(1).
//            s read(T<int>(i), &ch)
//        ].
//        
//        retVal value := v
//    ]
//}
//
//public extension formatterOp
//{
//    generic printFormatted(literal s, V<object> args)
//    [
//        auto buffer := TextBuilder new.
//        
//        int  len := s length.
//        int  i := 0.
//        char ch := $0.
//        int  chlen := 0.
//        int  index := 0.
//        
//        while (i < len)
//        [
//            s read(i, &ch).
//            
//            if (ch == $123)
//                [ 
//                    i := i + 1.
//                    
//                    helper readIndex(s, &i, &index).
//                    buffer write((args[index])~convertorOp toLiteral).
//                    
//                    chlen := 1.
//                ];
//                [ 
//                    buffer write(ch). 
//                    ch readLengthTo(&chlen)
//                ].
//            
//            i := i + chlen
//        ].
//        
//        self write:buffer.
//        
//        ^ self
//    ]
//    
//    generic printLineFormatted(literal s, V<object> args)
//    [
//        self~formatterOp printFormatted(s, args).
//        self writeLine.
//    ]
//}

// --- inputOp ---

public extension inputOp
{
    loadLineTo(var)
    {
        var line := self.readLine();
        
        line.convertTo:var;
        
        ^ var
    }
    
    loadLine(params object[] args)
    {
        string line := self.readLine();
        
        Array params := line.split($32);
        
        int len := args.Length;
        for(int i := 0, i < len, i := i + 1)
        {
            params[i].convertTo(args[i]);
        };
        
        ^ self
    }
}
