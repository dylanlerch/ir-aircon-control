var request = require("request");
var Service, Characteristic;

module.exports = function(homebridge) {
  Service = homebridge.hap.Service;
  Characteristic = homebridge.hap.Characteristic;
  
  homebridge.registerAccessory("homebridge-ac-over-http", "AirConditioner", AirConditioner);
}

function AirConditioner(log, config) {
  this.log = log;
  this.name = config["name"];
  this.url = config["server_url"];
  
  // Keep a record of the current temperature around. This is not a source of
  // truth, but just a hint for calculating if the system is heating or cooling.
  this.temperaturehint = 24;

  this.service = new Service.Thermostat(this.name);

  this.log("URL is set to: " + this.url);
  
  this.service
    .getCharacteristic(Characteristic.CurrentHeatingCoolingState)
    .on('get', this.getCurrentState.bind(this));

  this.service
    .getCharacteristic(Characteristic.TargetHeatingCoolingState)
    .on('get', this.getTargetState.bind(this))
    .on('set', this.setTargetState.bind(this));

  this.service
    .getCharacteristic(Characteristic.CurrentTemperature)
    .on('get', this.getTemperature.bind(this));

  this.service
    .getCharacteristic(Characteristic.TargetTemperature)
    .on('get', this.getTemperature.bind(this))
    .on('set', this.setTemperature.bind(this));

  this.service
    .getCharacteristic(Characteristic.TemperatureDisplayUnits)
    .on('get', this.getTemperatureDisplayUnits.bind(this));
}

AirConditioner.prototype.getCurrentState = function(callback) {
  request.get({
    url: this.url + "/status"
  }, function(err, reponse, body) {
    if (!err && reponse.statusCode == 200) {
      var json = JSON.parse(body);
      var active = json.on;
      var temperature = json.temp;

      this.temperaturehint = temperature;

      if (active === true) {
         // It's always running in auto, so if temp is >= 27, say it's heating.
         // otherwise, say it's cooling.
        if (temperature >= 27) {
          this.log("Current state: Heating");
          callback(null, Characteristic.CurrentHeatingCoolingState.HEAT)
        } else {
          this.log("Current state: Cooling");
          callback(null, Characteristic.CurrentHeatingCoolingState.COOL)
        }
      } else {
        this.log("Current state: Off");
        callback(null, Characteristic.CurrentHeatingCoolingState.OFF)
      }
    } else {
      this.log("Error getting current state");
      callback(err);
    }
  }.bind(this));
}

AirConditioner.prototype.getTargetState = function(callback) {
  request.get({
    url: this.url + "/status"
  }, function(err, reponse, body) {
    if (!err && reponse.statusCode == 200) {
      var json = JSON.parse(body);
      var active = json.on;

      if (active === true) {
        this.log("Target state: Auto");
        callback(null, Characteristic.TargetHeatingCoolingState.AUTO)
      } else {
        this.log("Target state: Off");
        callback(null, Characteristic.TargetHeatingCoolingState.OFF)
      }
    } else {
      this.log("Error getting target state");
      callback(err);
    }
  }.bind(this));
}

AirConditioner.prototype.setTargetState = function(state, callback) {
  var url = this.url;
  
  if (state === Characteristic.TargetHeatingCoolingState.OFF)  {
    url += "/off"
  } else { // for all other states (HEAT, COOL and AUTO), just use AUTO
    url += "/on"
  }

  request.get({
    url: url
  }, function(err, reponse, body) {
    if (!err && reponse.statusCode == 200) {
      // Success, so update current state. Can't set current state to auto,
      // so use the temperature hint to work out it heating or cooling
      var currentState;

      if (state === Characteristic.TargetHeatingCoolingState.OFF) {
        this.service
          .setCharacteristic(Characteristic.CurrentHeatingCoolingState, Characteristic.CurrentHeatingCoolingState.OFF);
      } else {
        this.log(this.temperaturehint);

        if (this.temperaturehint >= 27) {
          this.service
            .setCharacteristic(Characteristic.CurrentHeatingCoolingState, Characteristic.CurrentHeatingCoolingState.HEAT);
        } else {
          this.service
            .setCharacteristic(Characteristic.CurrentHeatingCoolingState, Characteristic.CurrentHeatingCoolingState.COOL);
        }
      }
      
      callback(null);
    } else {
      this.log("Error setting target state: " + err);
      callback(err);
    }
  }.bind(this));
}

AirConditioner.prototype.getTemperature = function(callback) {
  request.get({
    url: this.url + "/status"
  }, function(err, reponse, body) {
    if (!err && reponse.statusCode == 200) {
      var json = JSON.parse(body);
      var temperature = json.temp;

      this.temperaturehint = temperature;

      this.log("Current temperature: " + temperature);
      callback(null, parseInt(temperature));
    } else {
      this.log("Error getting tempterature");
      callback(err);
    }
  }.bind(this));
}

AirConditioner.prototype.setTemperature = function(temperature, callback) {
  this.log("Set temperature to " + temperature);
  
  // Ensure the temperature is in the correct range, otherwise the web server
  // will throw 404s
  if (temperature > 30) {
    temperature = 30;
  } else if (temperature < 17) {
    temperature = 17;
  }

  this.temperaturehint = temperature;

  var url = this.url + "/" + temperature;

  request.get({
    url: url
  }, function(err, reponse, body) {
    if (!err && reponse.statusCode == 200) {
      // Success, so update current temperature
      this.service
        .setCharacteristic(Characteristic.CurrentTemperature, temperature);

      callback(null);
    } else {
      this.log("Error setting temperature: " + err);
      callback(err);
    }
  }.bind(this));
}

AirConditioner.prototype.getTemperatureDisplayUnits = function(callback) {
  // Always celsius, because we're not animals
  callback(null, Characteristic.TemperatureDisplayUnits.CELSIUS);
}

AirConditioner.prototype.getServices = function() {
  return [this.service];
}