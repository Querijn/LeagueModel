<?php
header('Content-Type: application/json');
$cache = "small-summary.json";
if (file_exists($cache))
    die(file_get_contents($cache));

$json = json_decode(file_get_contents("champion-summary.json"), true);
$result = Array();
foreach ($json["data"] as &$data) 
{
    $champion = Array();
    $champion["id"] = $data["id"];
    $champion["key"] = $data["key"];
    $champion["name"] = $data["name"];
    $champion["skins"] = $data["skins"];

    $result[] = $champion;
}

$text = json_encode($result);
file_put_contents($cache, $text);
echo $text;