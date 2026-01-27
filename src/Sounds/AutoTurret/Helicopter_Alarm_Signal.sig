AudioSignalResClass {
 Inputs {
  IOPItemInputClass {
   id 4
   name "AutoTurretAlarmPitch"
   tl 352 -160
   children {
    11
   }
   value -5.6
   valueMin -6
   valueMax 6
   global 1
  }
 }
 Ops {
  SignalOpSt2GainClass {
   id 11
   name "St2Gain 11"
   tl 608 -160
   children {
    8
   }
   inputs {
    ConnectionClass "4:0" {
     id 4
     port 0
    }
   }
  }
 }
 Outputs {
  IOPItemOutputClass {
   id 8
   name "Pitch"
   tl 832 -160
   input 11
  }
 }
 compiled IOPCompiledClass "{687683BF9CA8D81A}" {
  visited {
   261 133 391 11 263 5 135 6
  }
  ins {
   IOPCompiledIn "{687683BF9CA8D803}" {
    data {
     1 65539
    }
   }
   IOPCompiledIn "{687683BF9CA8D878}" {
    data {
     1 196611
    }
   }
   IOPCompiledIn "{687683BF9CA8D877}" {
    data {
     1 3
    }
   }
  }
  ops {
   IOPCompiledOp "{687683BF9CA8D869}" {
    data {
     1 131075 4 196609 0 131072 0
    }
   }
   IOPCompiledOp "{687683BF9CA8D861}" {
    data {
     1 2 2 0 0
    }
   }
   IOPCompiledOp "{687683BF9CA8D85E}" {
    data {
     0 2 1 0
    }
   }
   IOPCompiledOp "{687683BF9CA8D85A}" {
    data {
     1 3 2 65536 0
    }
   }
  }
  outs {
   IOPCompiledOut "{687683BF9CA8D84E}" {
    data {
     0
    }
   }
  }
  processed 8
  version 2
 }
 Input_Order {
  ItemDetailListItemClass AutoTurretAlarmPitch {
   Name "AutoTurretAlarmPitch"
   Id 4
  }
 }
 Output_Order {
  ItemDetailListItemClass Pitch {
   Name "Pitch"
   Id 8
  }
 }
}