class CFTime
{
    private:
    int _hour;
    int _minute;

    public:

    CFTime()
    {
        _hour = 0;
        _minute = 0;
    }    

    CFTime(int hour, int minute)
    {
        _hour = hour;
        _minute = minute;
    }

    void SetTime(int hour, int minute)
    {
        _hour = hour;
        _minute = minute;
    }

    int Hour()
    {
        return _hour;
    }

    int Minute()
    {
        return _minute;
    }

    String ToString()
    {
        String result = String(_hour);
        result += ":";
        result += String(_minute);
        return result;
    }

    bool IsTime(int hour, int minute)//Comparing and logical summing
    {        
        return (_hour==hour)&&(_minute==minute);
    }

};