characterFsm = {
    Idle = {
        {type="VariableEqualLink", targetState="Walk", variableName="TryingToMove", value=true},
        {type="VariableEqualLink", targetState="Shoot", variableName="TryingToShoot", value=true},
    },
    Walk = {
        {type="VariableEqualLink", targetState="Idle", variableName="TryingToMove", value=false},
        {type="VariableEqualLink", targetState="WalkAndShoot", variableName="TryingToShoot", value=true},
        {type="VariableEqualLink", targetState="Run", variableName="ReadyToRun", value=true},
    },
    Shoot = {
        {type="VariableEqualLink", targetState="WalkAndShoot", variableName="TryingToMove", value=true},
        {type="VariableEqualLink", targetState="Idle", variableName="TryingToShoot", value=false},
    },
    WalkAndShoot = {
        {type="VariableEqualLink", targetState="Shoot", variableName="TryingToMove", value=false},
        {type="VariableEqualLink", targetState="Walk", variableName="TryingToShoot", value=false},
        {type="VariableEqualLink", targetState="Run", variableName="ReadyToRun", value=true},
    },
    Run = {
        {type="VariableEqualLink", targetState="Walk", variableName="ReadyToRun", value=false},
        {type="VariableEqualLink", targetState="Idle", variableName="TryingToMove", value=false},
    },
}
