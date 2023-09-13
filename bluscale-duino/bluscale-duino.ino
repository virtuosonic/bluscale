/*******************************************
Nombre: 	bluscale-duino.ino
Autor:  	Gabriel Espinoza
Fecha:  	29-Oct-2018
Desc:   	Firware para bascula bluetooth, 
		basada en el ATMega328p
License:	MIT
*/

/*
La bascula emula una arduino pro mini (3.3V 8MHz)
configurar los fusibles asi:
        low_fuses=0xFF
        high_fuses=0xDA
        extended_fuses=0xFF
        lock=0xC0

Para configurar el HM-10, conectarse y ejecutar los
siguientes comandos AT:
        AT+NAMES1_SCALE
        AT+UUID0x0EB7
        AT+CHAR0x0EB8
        AT+PIO11
        AT+POWE3

*/

#include <HX711.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

#define __DEBUG 0

#if __DEBUG
    #define LOGDBG(x) Serial.println(x)
#else
    #define LOGDBG(x) asm("nop")
#endif

/**Estados de carga de la bascula
 * */
enum
{
    CHARGING,///< Cargando
    CHRGTERM,///< Carga terminada
    DISCHRG,///< Descargando
    NOBATT,///< Sin bateria
    LOWBATT ///< Bateria baja
};

/**textos que retorna la
 * bascula para notificar
 * los estados de carga
 */
const char notifs[][4] =
{
    "ch",
    "cht",
    "dch",
    "nb",
    "lb"
};

/**Version de Hardware*/
const unsigned long versionCode = 1000;
/**Ganancia del hx711*/
const unsigned LC_GAIN = 64;
const unsigned PAUSE = 0;
const float DEFCAL = 1;
///pines hx711
const unsigned LC_DOUT = 3;
const unsigned LC_CLK = 2;
///pines HM-10
const unsigned RX = 7;
const unsigned TX = 8;
const unsigned CONNPIN = 9;
///pines monitor bateria
const unsigned CHRGPIN = 10;
const unsigned FCHPIN = 6;
const int BATTV = A1;
///eeprom
const int calADDR = 0;
const int initADDR = calADDR + sizeof(float);

/** Filtro de deriva.
 * Durante el uso de la bascula
 * por efecto del calentamiento
 * y otros factores una bascula
 * sin carga no se mantiene en
 * cero. Este filtro ajusta el
 * OFFSET para corregir este
 * comportamiento.
 */
class DriftFilter
{
public:
    /** Contructor default*/
    DriftFilter() :
        count(0)
    {

    }
    /** Retorna la diferencia entre el parametro
     * y el promedio de lecturas almacenadas
     * y post-agrega una lectura al array
     */
    float add(float f)
    {
        //calcular error
        auto err = f - avg();
        auto abserr = fabs(err);
        //agregar a array
        shift();
        _arr[0] = f;
        //evaluar
        if (abserr > 0.04 && abserr < 0.1)
        {
            return err;
        }
        else
        {
            return 0;
        }
    }
private:
    /** Recorre los datos en el array*/
    void shift()
    {
        for (unsigned i = 0; i < count ; ++i)
        {
            _arr[i+1] = _arr[i];
        }
    }
    /** Retorna el promedio del array*/
    float avg()
    {
        float acumulator = 0;
        for (unsigned i = 0 ; i < count; ++i)
        {
            acumulator += _arr[i];
        }
        return acumulator / count;
    }
    /** array que almacena las lecturas*/
    float _arr[10];
    /** Cantidad de elementos en el array*/
    unsigned count;
};

class ScaleCtl
{
public:
    ScaleCtl(HX711* scale,SoftwareSerial* serial) :
        streaming(false),
        notify(false),
        filter(true),
        READS(10),
        pstat(DISCHRG)
    {
        _scale = scale;
        _serial = serial;
    }
    void Start()
    {
        streaming = true;
        _scale->power_up();
    }
    void Stop()
    {
        streaming = false;
        _scale->power_down();
    }
    void Calibrate(float cal)
    {
        if (!streaming)
        {
            _scale->power_up();
        }
        cal = _scale->get_value(READS) / cal;
        EEPROM.put(calADDR, cal);
        _scale->set_scale(loadScale());
        if (!streaming)
        {
            _scale->power_down();
        }
    }
    float battLevel()
    {
        ///el voltaje de la bateria es atenuado
        ///por un voltage divider antes de
        ///entrar a A1 para ser leido
        //4.2 / 1023  = ‭0.0041055718475073‬
        //1.3 * 100 = 1300
        return (0.0041055718475073 * analogRead(BATTV)- 2.9 ) / 1300;
    }
    /**
    Retorna la escala (valor por el cual se multiplica
    la lectura de la bascula para obtener la escala requerida)
    guardada en la EEPROM
    */
    float loadScale()
    {
        float ffff = 0;
        EEPROM.get(calADDR, ffff);
        if (ffff == 0)
        {
            ffff = 1 ;
        }
        return ffff;
    }
    void setNotify(bool n)
    {
        notify = n;
    }
    void setFilter(bool b)
    {
        filter = b;
    }
    void loop()
    {
        if (streaming)
            Stream();
        if (notify)
            Notify();
    }
    void setReads(unsigned r)
    {
        if (r <= 0)
        {
            LOGDBG("ScaleCtl::setReads: param must be positive");
            return;
        }
        READS = r;
    }
    void adjust(float err)
    {
        auto offset = _scale->get_offset();
        auto correction = err * _scale->get_scale() + offset;
        _scale->set_offset(correction);
    }
private:
    void Stream()
    {
        if(versionCode > 1000 && digitalRead(CONNPIN) == LOW)
        {
            Stop();
        }
        else
        {
            // leer
            float fffff = _scale->get_units(READS);
            //filtrar
            if (filter)
            {
                auto correction = _filter.add(fffff);
                if (correction)
                {
                    //corregir si es necesario
                    adjust(correction);
                }
            }
            // enviar a BLE
            _serial->println(fffff, 1);
        }
    }
    void Notify()
    {
        unsigned chrgPin = digitalRead(CHRGPIN);
        unsigned chrgStbyPin = digitalRead(FCHPIN);
        unsigned chrgState = 1000;
        //detectar estado del cargador
        // pin7 && 6 == HIGH
        if (chrgPin && chrgStbyPin)
        {
            chrgState = DISCHRG;
        }
        //pin 7 == HIGH && 6 == LOW
        else if (chrgPin && !chrgStbyPin)
        {
            chrgState = CHRGTERM;
        }
        //pin 7 ==  LOW && 6 == HIGH
        else if (!chrgPin && chrgStbyPin)
        {
            chrgState = CHARGING;
        }
        //pin 7 ==  LOW && 6 == LOW
        else if (!chrgPin && !chrgStbyPin)
        {
            chrgState = NOBATT;
        }
        //detectar si hubo cambio de estado
        if (pstat != chrgState)
        {
            pstat = chrgState;
            _serial->println(notifs[pstat]);
        }
        if (battLevel() < 10)
        {
            _serial->println(notifs[LOWBATT]);
        }
    }
    DriftFilter _filter;
    HX711* _scale;
    SoftwareSerial* _serial;
    bool streaming;
    bool notify;
    bool filter;
    unsigned READS;
    unsigned pstat;
};

class ScaleCmd
{
public:
    ScaleCmd(ScaleCtl* ctl,HX711* scale)
    {
        _ctl = ctl;
        _scale = scale;
    }
    void cmd(const String& data)
    {
        if (data.equals("s"))
        {
            _ctl->Start();
        }
        ///finish
        else if (data.equals("f"))
        {
            _ctl->Stop();
        }
        ///tare
        else if (data.equals("t"))
        {
            _scale->tare();
        }
        //activar notificaciones
        else if (data.equals("nt"))
        {
            _ctl->setNotify(false);
        }
        //apagar notificaciones
        else if (data.equals("nnt"))
        {
            _ctl->setNotify(false);
        }
        else if (data.equals("fl"))
        {
            _ctl->setFilter(true);
        }
        else if (data.equals("nfl"))
        {
            _ctl->setFilter(false);
        }
        ///calibration
        else if (data.startsWith("c"))
        {
            float fff = 0;
            fff = data.substring(data.indexOf(" ")).toFloat();
            if (fff != 0)
            {
                _ctl->Calibrate(fff);
            }
            else
            {
                _serial->print("C=");
                _serial->println(_scale->get_scale());
            }
        }
        else if(data.startsWith("avg"))
        {
            float avgr = 0;
            avgr = data.substring(data.indexOf(" ")).toInt();
            if(avgr != 0)
            {
                _ctl->setReads(avgr);
            }
        }
        else if (data.equals("bluscale"))
        {
            _serial->print("bluscale=");
            _serial->println(versionCode);
        }
        else if (data.equals("batt") && versionCode > 1000)
        {
            _serial->print("B=");
            _serial->println(_ctl->battLevel(), 2);
        }
    }
private:
    ScaleCtl* _ctl;
    HX711* _scale;
    SoftwareSerial* _serial;
};


HX711 bascula;
SoftwareSerial btSerial(RX, TX);
ScaleCtl ctl(&bascula,&btSerial);
ScaleCmd cmd(&ctl,&bascula);

/**Punto de entrada
*/
void setup()
{
#if __DEBUG
    Serial.begin(9600);
    while (!Serial)
    {

    }
#endif // __DEBUG
    btSerial.begin(9600);
    // TODO (admin#1#): buscar el minimo
    delay(1000);
    LOGDBG("starting scale");
    bascula.begin(LC_DOUT, LC_CLK, LC_GAIN);
    bascula.set_scale(ctl.loadScale());
    bascula.tare();
    bascula.power_down();
    ///la referencia interna de atmega328p es de 1.1V
    analogReference(INTERNAL);
    pinMode(A1,INPUT);
    pinMode(CONNPIN,INPUT);
    pinMode(CHRGPIN,INPUT_PULLUP);
    pinMode(FCHPIN,INPUT_PULLUP);
}

/**ciclo sin fin
*/
void loop()
{
#if __DEBUG
    if (Serial.available())
    {
        btSerial.println(Serial.readString());
    }
#endif // __DEBUG
    ///leer puerto serie
    if (btSerial.available())
    {
        String data = btSerial.readString();
        LOGDBG(data);
        data.trim();
        ///start
        cmd.cmd(data);//nami
    }
    ///envia la lectura de la celda de carga
    ///como texto con dos decimales
    ctl.loop();
    //pausa
    delay(PAUSE);
}
