# rrdtool create .rrd

sensors = Dict("Karttula/Olohuone/T/" => "karttula_oh_t.rrd",
               "Karttula/Olohuone/RH/" => "karttula_oh_rh.rrd",
               "Karttula/Olohuone/P/" => "karttula_oh_p.rrd",
               "Karttula/WC/T/" => "karttula_wc_t.rrd",
               "Karttula/WC/RH/" => "karttula_wc_rh.rrd")


st, pr = open(`/usr/bin/mosquitto_sub -h kw.dy.fi -u valpuri -P ammu1001 -v -t "#"`, "r", STDOUT)

while true
    addr, value = split(readline(st), " ")
    if haskey(sensors, addr)
        file = sensors[addr]
        run(`/usr/bin/rrdtool update $file N:$value`)
        println("/usr/bin/rrdtool update $file N:$value")
    end
end

    # rrdtool create karttula.rrd \
    #         --step 30 \
    #         DS:karttula_oh_t:GAUGE:600:U:U \
    #         DS:karttula_oh_rh:GAUGE:600:U:U \
    #         DS:karttula_oh_p:GAUGE:600:U:U \
    #         DS:karttula_wc_t:GAUGE:600:U:U \
    #         DS:karttula_wc_rh:GAUGE:600:U:U \
    #         RRA:AVERAGE:0.5:1:120 \
    #         RRA:AVERAGE:0.5:10:288 \
    #         RRA:AVERAGE:0.5:30:672 \
    #         RRA:AVERAGE:0.5:120:720 \
    #         RRA:AVERAGE:0.5:360:2920 \
    #         RRA:AVERAGE:0.5:720:4380 

    # One hour in 30s res, 120 = 60*60/30
    # One day in 5 min res, 5 mins =  5*60/30 = 10, 24*60/5 = 288
    # One week in 15 min res, 15 mins = 15*60/30 = 30, 7*24*60/15 = 672
    # One month in 1 hour res, 1 hours = 60*60/30 = 120, 30*24 = 720
    # One year in 3 hour res, 3 hours = 3*60*60/30 = 360, 365*24/3 = 2920
    # Three years in 6 hour res, 6 hours = 6*60*60/30 = 720, 3*365*24/6 = 4380
