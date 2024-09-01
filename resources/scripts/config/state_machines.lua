characterFsm = {
    Idle = {
        {type="VariableEqualLink:bool", targetState="Walk", variableName="TryingToMove", value=true},
        {type="VariableEqualLink:bool", targetState="Shoot", variableName="TryingToShoot", value=true},
    },
    Walk = {
        {type="VariableEqualLink:bool", targetState="Idle", variableName="TryingToMove", value=false},
        {type="VariableEqualLink:bool", targetState="WalkAndShoot", variableName="TryingToShoot", value=true},
        {type="VariableEqualLink:bool", targetState="Run", variableName="ReadyToRun", value=true},
    },
    Shoot = {
        {type="VariableEqualLink:bool", targetState="WalkAndShoot", variableName="TryingToMove", value=true},
        {type="VariableEqualLink:bool", targetState="Idle", variableName="TryingToShoot", value=false},
    },
    WalkAndShoot = {
        {type="VariableEqualLink:bool", targetState="Shoot", variableName="TryingToMove", value=false},
        {type="VariableEqualLink:bool", targetState="Walk", variableName="TryingToShoot", value=false},
        {type="VariableEqualLink:bool", targetState="Run", variableName="ReadyToRun", value=true},
    },
    Run = {
        {type="VariableEqualLink:bool", targetState="Walk", variableName="ReadyToRun", value=false},
        {type="VariableEqualLink:bool", targetState="Idle", variableName="TryingToMove", value=false},
    },
}
