//Datetime-like and pseudo-RTC class
class CFTime
{
    public:
    struct TimeStamp
    {
        byte Hour;
        byte Minute;
        byte Second;
    };
    

    private:
    TimeStamp _time;
    byte _lastSeconds;

    public:

    CFTime()
    {
        _time = {0, 0, 0};
        _lastSeconds = 0;
    }    

    CFTime(byte hour, byte minute, byte second)
    {
        _time = {hour, minute, second};
        _lastSeconds = 0;
    }

    CFTime(TimeStamp time)
    {
        _time = time;
    }

    void SetTime(byte hour, byte minute, byte second)
    {
        _time = {hour, minute, second};
    }

    void SetTime(TimeStamp time)
    {
        _time = time;
    }

    int Hour()
    {
        return _time.Hour;
    }

    int Minute()
    {
        return _time.Minute;
    }

    int Second()
    {
        return _time.Second;
    }

    TimeStamp Time()
    {
        return _time;
    }

    String ToString()
    {
        String result = String(_time.Hour);
        result += ":";
        result += String(_time.Minute);
        result += ":";
        result += String(_time.Second);
        return result;
    }

    bool IsTime(byte hour, byte minute, byte second)//Comparing and logical summing
    {        
        return (_time.Hour == hour)&&(_time.Minute == minute)&&(_time.Second == second);
    }

    bool IsTime(TimeStamp time)
    {
        return (_time.Hour == time.Hour)&&(_time.Minute == time.Minute)&&(_time.Second==time.Second);
    }

    bool Period(TimeStamp time)//For update with time as periodic value, not a stamp
    {   
        //For cases like 0:2:0 we are check time.
        //Ignoring incoming null time by setting true - null mean every time
        //Ignoring internal null time by setting true - null mean every time
        bool hFlag = true;
        bool mFlag = true;
        bool sFlag = true;     
        
        //The null leftower of the division are true, other cases are not in period
        if((_time.Hour    != 0)||(time.Hour    != 0)) hFlag = _time.Hour   % time.Hour   == 0;
        if((_time.Minute  != 0)||(time.Minute  != 0)) mFlag = _time.Minute % time.Minute == 0;
        if((_time.Second  != 0)||(time.Second  != 0)) sFlag = _time.Second % time.Second == 0;

        return hFlag&&mFlag&&sFlag;
    }

    void Tick()
    {
        //From UC boot we start millis, every cycle it's grow in limit of one minute
        byte nowSeconds = millis()/1000;
        _time.Second = nowSeconds - _lastSeconds;
        //Replace to math functions?
        if(_time.Second == 60) // Where the one minute is reached we fill the last seconds for count new
        {
            _lastSeconds = nowSeconds;
            _time.Minute++;
        }
        if(_time.Minute == 60)//Passing the one hour limit is reset the minute count
        {
            _time.Minute = 0;
            _time.Hour++;
        }
        if(_time.Hour == 24) //One day 
        {
            _time.Hour = 0;
        }
    }

};