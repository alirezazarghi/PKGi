<?php

// Database settings

// Create a table "packages" with inside :
// - id       | int          | primary keys
// - name     | varchar(255) | N/A
// - ico_link | varchar(255) | N/A
// - pkg_link | varchar(255) | N/A
// Note : N/A = Nothing to do

$pdo = new PDO('mysql:host=localhost;dbname=database_name', 'username', 'password');

switch ($_GET['do']) {
	case "info": {
		header("Content-Type:application/json");
		$data = array(
			"name" => "Your servers name here !"
		);
		$json_data = json_encode($data, JSON_NUMERIC_CHECK);
		header("Content-length: " . strlen($json_data));
		echo $json_data;
		break;
	}

	case "icons": {
		header('Content-Type: image/png');

		$id = intval($_GET['id']);
		$req = $pdo->prepare("SELECT * FROM packages WHERE id = ?");
		$req->bindParam(1, $id);
		$req->execute();

		$data = $req->fetchAll(PDO::FETCH_OBJ);
		if (count($data) > 0) {
			$file = "./icons/".$data[0]->ico_link;
			if (file_exists($file)) {
			    header('Content-Length: ' . filesize($file));
			    readfile($file);
			}
		} else {
			header('Content-Length: 0');
		}

		break;
	}

	case "downloads": {
		header('Content-Type: application/octet-stream');

		$id = intval($_GET['id']);
		$req = $pdo->prepare("SELECT * FROM packages WHERE id = ?");
		$req->bindParam(1, $id);
		$req->execute();

		$data = $req->fetchAll(PDO::FETCH_OBJ);
		if (count($data) > 0) {
			$file = "./pkgs/".$data[0]->pkg_link;
			if (file_exists($file)) {
			    header('Content-Length: ' . filesize($file));
			    readfile($file);
			}
		} else {
			header('Content-Length: 0');
		}

		break;
	}

	case "packages": {
		header("Content-Type:application/json");
		$line = 5;
		$page = 0;

		if ($_GET['line']) {
			$line = intval($_GET['line']);
		}

		if ($_GET['page']) {
			$page = intval($_GET['page']);
			$startfrom = ($_GET['page'] - 1) * $line;
		} else {
			$page = 0;
			$startfrom = 0;
		}

		$total_nbr = 0;

		if (!$_GET['search']) {
			$req = $pdo->prepare("SELECT COUNT(*) FROM packages");
			$req->execute();
			$total_nbr = $req->fetchColumn();

			$req = $pdo->prepare("SELECT * FROM packages ORDER BY id ASC LIMIT ?, ?");
			$req->bindParam(1, intval(trim($startfrom)), PDO::PARAM_INT);
			$req->bindParam(2, intval(trim($line)), PDO::PARAM_INT);
		} else {
			echo "search";
			$req = $pdo->prepare("SELECT COUNT(*) FROM packages WHERE name LIKE ?");
			$req->bindParam(1, $_GET['search']);
			$req->execute();

			$total_nbr = $req->fetchColumn();

			$req = $pdo->prepare("SELECT * FROM packages ORDER BY id ASC LIMIT ?, ? WHERE name LIKE ?");
			$req->bindParam(1, intval(trim($startfrom)), PDO::PARAM_INT);
			$req->bindParam(2, intval(trim($line)), PDO::PARAM_INT);
			$req->bindParam(3, $_GET['search']);

		}

		$req->execute();
		$data = $req->fetchAll(PDO::FETCH_OBJ);
		$data = array(
			"total" => $total_nbr,
			"page" => $page,
			"line" => $line,
			"packages" => $data
		);

		$json_data = json_encode($data, JSON_NUMERIC_CHECK);
		header("Content-length: " . strlen($json_data));
		echo $json_data;
		break;
	}
}
?>
