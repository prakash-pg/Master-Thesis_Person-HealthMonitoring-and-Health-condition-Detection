// Create the charts when the web page loads
window.addEventListener('load', onload);

function onload(event){
  chartBT = createBodyTemperatureChart();
  chartHR = createHeartRateChart();
}

// Create BodyTemperature Chart
function createBodyTemperatureChart() {
  var chart = new Highcharts.Chart({
    chart:{ 
      renderTo:'chart-bodytemperature',
      type: 'spline' 
    },
    series: [
      {
        name: 'LM 35'
      }
    ],
    title: { 
      text: undefined
    },
    plotOptions: {
      line: { 
        animation: false,
        dataLabels: { 
          enabled: true 
        }
      }
    },
    xAxis: {
      type: 'datetime',
      dateTimeLabelFormats: { second: '%H:%M:%S' }
    },
    yAxis: {
      title: { 
        text: 'Temperature Celsius Degree' 
      }
    },
    credits: { 
      enabled: false 
    }
  });
  return chart;
}

// Create Temperature Chart
function createHeartRateChart() {
  var chart = new Highcharts.Chart({
    chart:{ 
      renderTo:'chart-heartrate',
      type: 'spline' 
    },
    series: [
      {
        name: 'MAX 30102'
      }
    ],
    title: { 
      text: undefined
    },
    plotOptions: {
      line: { 
        animation: false,
        dataLabels: { 
          enabled: true 
        }
      }
    },
    xAxis: {
      type: 'datetime',
      dateTimeLabelFormats: { second: '%H:%M:%S' }
    },
    yAxis: {
      title: { 
        text: 'Beats Per Minute' 
      }
    },
    credits: { 
      enabled: false 
    }
  });
  return chart;
}

