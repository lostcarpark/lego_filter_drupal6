<?php

class BrickCatApi {
  private $api_key;
  
  public function __construct($api_key) {
    $this->api_key = $api_key;
  }
  
  public function getBestPrice($set, $language) {
    $url = "https://api.brickcat.com/set/bestPrice/$set/$language";
    return $this->get($url, []);
  }
  
  public function getLegoUrl($set, $language) {
    $url = "https://api.brickcat.com/set/legoUrl/$set/$language";
    return $this->get($url, []);
  }
  
  public function get($url, $params=[]) {
    $gets = [];
    foreach ($params as $key=>$param) {
      $gets[] = $key.'='.$param;
    }
    $paramString = implode('&', $gets);
    return $this->request('GET', $url.'?'.$paramString, []);
  }
  
  public function request($method, $url, $params) {
    // Create a stream
    $opts = [
      'http'=>[
        'method'=>$method,
        'header'=>"key: ".$this->api_key."\r\n"
      ]
    ];
    $context = stream_context_create($opts);

    // Open the file using the HTTP headers set above
    $file = file_get_contents($url, false, $context);
    $result = json_decode($file);
    return $result;
  }
  
}
