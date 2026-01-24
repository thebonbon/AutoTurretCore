AudioSignalResClass {
 Inputs {
  IOPInputCurveModulatorClass {
   id 20
   name "Curve Modulator 20"
   tl 396.293 -3.184
   children {
    2
   }
   Points {
    IOPCurveModulatorPoint a {
     Time 100
     Value 3
    }
    IOPCurveModulatorPoint b {
    }
    IOPCurveModulatorPoint c {
     Value -3
    }
   }
  }
 }
 Outputs {
  IOPItemOutputClass {
   id 2
   name "Pitch"
   tl 624 32
   input 20
  }
 }
 Output_Order {
  ItemDetailListItemClass Pitch {
   Name "Pitch"
   Id 2
  }
 }
}