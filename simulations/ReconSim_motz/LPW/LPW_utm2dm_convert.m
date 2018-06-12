load 'H:\ReconSim\LPW_REMUS_Run.txt'


zone = repmat('8  V', 12,1)
[latDd lonDd] = utm2deg(LPW_REMUS_Run(:,1), LPW_REMUS_Run(:,2), zone)

latDm = degrees2dm(latDd)

lonDm = degrees2dm(lonDd)

LPW_runDm = [latDm lonDm]

save 'H:\ReconSim\LPW\LPW_run.txt' LPW_runDm -ascii