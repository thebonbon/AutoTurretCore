AudioSignalResClass {
 Inputs {
  IOPItemInputClass {
   id 4
   name "TrackingState"
   tl -336.548 -152.617
   children {
    7
   }
   value 1
  }
  IOPItemInputClass {
   id 5
   name "DamageState"
   tl -329.636 157.664
   children {
    10
   }
   value 0.1
  }
  IOPItemInputClass {
   id 6
   name "EngineOn"
   tl -329.636 51.463
   children {
    3
   }
   value 1
  }
 }
 Ops {
  IOPItemOpMulClass {
   id 3
   name "Engine + Dmg"
   tl 136.017 151.353
   children {
    9
   }
   inputs {
    ConnectionClass connection {
     id 10
     port 0
    }
    ConnectionClass connection {
     id 6
     port 0
    }
   }
  }
  IOPItemOpConvertorClass {
   id 7
   name "Tracked?"
   tl -55.54 -159.684
   children {
    8
   }
   inputs {
    ConnectionClass connection {
     id 4
     port 0
    }
   }
   Intervals {
    IOPItemOpConvertorRange Intermediary {
     min 1
     max 2
     out 250
    }
   }
  }
  IOPItemOpMulClass {
   id 9
   name "Track + Engine + Dmg"
   tl 464.477 -77.093
   inputs {
    ConnectionClass connection {
     id 3
     port 0
    }
   }
  }
  IOPItemOpConditionClass {
   id 10
   name "IsDestroyed?"
   tl -97.496 159.26
   children {
    3
   }
   inputs {
    ConnectionClass connection {
     id 5
     port 0
    }
   }
   "Condition Type" "!="
   Comparator 2
  }
 }
 Outputs {
  IOPItemOutputClass {
   id 8
   name "Tracking_Time"
   tl 733.365 -156.343
   input 7
  }
 }
 compiled IOPCompiledClass {
  visited {
   261 133 391 11 263 5 135 6
  }
  ins {
   IOPCompiledIn {
    data {
     1 65539
    }
   }
   IOPCompiledIn {
    data {
     1 196611
    }
   }
   IOPCompiledIn {
    data {
     1 3
    }
   }
  }
  ops {
   IOPCompiledOp {
    data {
     1 131075 4 196609 0 131072 0
    }
   }
   IOPCompiledOp {
    data {
     1 2 2 0 0
    }
   }
   IOPCompiledOp {
    data {
     0 2 1 0
    }
   }
   IOPCompiledOp {
    data {
     1 3 2 65536 0
    }
   }
  }
  outs {
   IOPCompiledOut {
    data {
     0
    }
   }
  }
  processed 8
  version 2
 }
}