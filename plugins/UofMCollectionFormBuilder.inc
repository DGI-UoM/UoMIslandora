<?php

module_load_include('inc','fedora_repository','plugins/ModsFormBuilder');

class UofMCollectionFormBuilder extends ModsForMBuilder {
  
  private $pid; 
  
  function handleCollectionForm($form_values) {
    global $base_url;
    module_load_include('inc','fedora_repository','plugins/ModsFormBuilder');
    module_load_include('inc', 'fedora_repository', 'api/fedora_item');
    module_load_include('inc', 'fedora_repository', 'CollectionPolicy');
    module_load_include('inc', 'fedora_repository', 'plugins/ImageManipulation');

    $pid = trim($form_values['new_pid']);
    $form_values['pid']=$pid;
    $this->pid=$pid;
    
    if (!XMLDatastream::validPid($pid)) {	    
      drupal_set_message('Error: Invalid PID entered.  Please choose a different PID and try again.','error'); 
      return;
    } else {
      $item = new fedora_item($pid); 
      if ($item->objectProfile != NULL)  {
	drupal_set_message('Error: Specified PID already exists.  Please choose a different PID and try again.', 'error');
	return;
      }
    }

    $imageManip = new ImageManipulation();

    $expected_files = array('thumbnail'=>'TN',
			    'image1'=>'ROTATOR_IMG1',
			    'image2'=>'ROTATOR_IMG2',
			    'image3'=>'ROTATOR_IMG3',
			    'image4'=>'ROTATOR_IMG4',
			    'image5'=>'ROTATOR_IMG5',
			    'image6'=>'ROTATOR_IMG6');
    $resize_width = array('thumbnail' => 150, 'image1' => 500, 'image2' => 500, 'image3' => 500, 'image4' => 500, 'image5' => 500, 'image6' => 500);
    
    foreach ($expected_files as $name=>$dest_dsid) {
      $file = file_save_upload($name, array(
	'file_validate_extensions' => array('png gif jpg jpeg'),
	'file_validate_is_image' => array(),
      ));
      if (isset($file->filename)) {
	file_move($file,$thumbnail->filename);
	$imageManip->createPreview(array('width'=>$resize_width[$name]), $dest_dsid, $file->filepath, 'png');
	@unlink($file->filepath);
      }
    }
    $this->createRotatorQuery($form_values, $dom, $rootElement);


    $dom = new DomDocument("1.0", "UTF-8");
    $dom->formatOutput = TRUE;
    $rootElement = $dom->createElement("foxml:digitalObject");
    $rootElement->setAttribute('VERSION', '1.1');
    $rootElement->setAttribute('PID', "$pid");
    $rootElement->setAttribute('xmlns:foxml', "info:fedora/fedora-system:def/foxml#");
    $rootElement->setAttribute('xmlns:xsi', "http://www.w3.org/2001/XMLSchema-instance");
    $rootElement->setAttribute('xsi:schemaLocation', "info:fedora/fedora-system:def/foxml# http://www.fedora.info/definitions/1/0/foxml1-1.xsd");
    $dom->appendChild($rootElement);

    // Create standard fedora stuff
    $form_values['dc:title'] = $form_values['mods_title'];
    $this->createStandardFedoraStuff($form_values, $dom, $rootElement);

    // Create relationships
    $this->createRelationShips($form_values, $dom, $rootElement);
    $collectionPid = $form_values['collection_pid'];
	
    if (($cp = CollectionPolicy::LoadFromCollection($collectionPid)) !== FALSE) {
      $collectionName =trim($cp->getName()); 
      if (trim($collectionName)!='') {
	$form_values['dc_relation']=$collectionName;
      }
    }    
    // Create MODS
    $this->createModsStream($form_values, $dom, $rootElement);
    $this->createCollectionPolicy($form_values, $dom, $rootElement);    
    $this->createWorkflowStream($form_values, $dom, $rootElement);
    
    
    if (!empty($form_values['ingest-file-location']) || !empty($_SESSION['fedora_ingest_files'])) {
      $this->createFedoraDataStreams($form_values, $dom, $rootElement);
    }
    $this->createPolicy($collectionPid, &$dom, &$rootElement);

//       header('Content-type: application/xml');
//       echo $dom->saveXML(); exit();

    try {
	$object = Fedora_Item::ingest_from_FOXML($dom);
	//for some reason, ingest_from_FOXML does not generate a JMS message
	//I just modify the workflow DS and it sends a JMS message.  
	$item = new Fedora_Item($object->pid);
	$item->modify_datastream_by_value( $item->get_datastream_dissemination('WORKFLOW'), 'WORKFLOW', "Workflow Record", 'text/xml');

	if (!empty($object->pid)) {
	  drupal_set_message(t("Item !pid created successfully.", array('!pid' => l($object->pid, 'fedora/repository/'. $object->pid))), "status");
	 }
	 if (!empty( $_SESSION['fedora_ingest_files'])) {
	   foreach ($_SESSION['fedora_ingest_files'] as $dsid => $createdFile) {
	    file_delete($createdFile);
	   }
	 }
	 file_delete($form_values['ingest-file-location']);
    }
    catch (exception $e) {
      drupal_set_message(t('Error ingesting object: !e', array('!e' => $e->getMessage())), 'error');
      watchdog(t("Fedora_Repository"), t("Error ingesting object: !e", array('!e' => $e->getMessage())), NULL, WATCHDOG_ERROR);
      return;
    }		
  }
	
  function createCollectionPolicy($form_values, &$dom, &$rootElement) {
    module_load_include('inc', 'fedora_repository', 'api/fedora_item');
    module_load_include('inc', 'fedora_repository', 'ContentModel');
    module_load_include('inc', 'fedora_repository', 'CollectionPolicy');
    global $base_url;
    
    $datastream = $dom->createElement("foxml:datastream");
    $datastream->setAttribute("ID", "COLLECTION_POLICY");
    $datastream->setAttribute("STATE", "A");
    $datastream->setAttribute("CONTROL_GROUP", "X");
    $version = $dom->createElement("foxml:datastreamVersion");
    $version->setAttribute("ID", "COLLECTION_POLICY.0");
    $version->setAttribute("MIMETYPE", "text/xml");
    $version->setAttribute("LABEL", "Collection Policy");
    $datastream->appendChild($version);
    $content = $dom->createElement("foxml:xmlContent");
    $version->appendChild($content);
    $rootElement->appendChild($datastream);


    $cpRoot = $dom->createElement("collection_policy");
    $cpRoot->setAttribute('xmlns','http://www.islandora.ca');
    $cpRoot->setAttribute('xmlns:xsi','http://www.w3.org/2001/XMLSchema-instance');
    $cpRoot->setAttribute('name',trim($form_values['mods_title']));
    $cpRoot->setAttribute('xsi:schemaLocation','http://www.islandora.ca '.$base_url.'/collection_policy.xsd');
    $content->appendChild($cpRoot);
   
    $models = $dom->createElement('content_models');
    $cpRoot->appendChild($models);
    foreach ($form_values['allowed_models'] as $model) {
      if ( ($cm = ContentModel::loadFromModel($model))!== FALSE) { 
	$model = $dom->createElement('content_model');
	$model->setAttribute('dsid',$cm->dsid);
	$model->setAttribute('name',$cm->getName());
	$model->setAttribute('namespace',$this->pid);
	$model->setAttribute('pid',$cm->pid);
	$models->appendChild($model);
      }
    }
    
    $search_terms = $dom->createElement('search_terms'); 
    $cpRoot->appendChild($search_terms);
    
    $relationship = $dom->createElement('relationship','isMemberOfCollection');
    $cpRoot->appendChild($relationship);
    
    if ( ($cp = CollectionPolicy::loadFromCollection($form_values['collection_pid'])) !== FALSE) { 
      $dir=$cp->getStagingArea().'/'.preg_replace('/\:/',' ',$this->pid);
			
      if (!file_exists($dir)) { 
	 @mkdir($dir);
      }
      $staging_area = $dom->createElement('staging_area',$dir);
      $cpRoot->appendChild($staging_area);
    }
    
  }
  
  function createRotatorQuery($form_values, &$dom, &$rootElement) {

    
    switch ($form_values['rotator']) {
      case 'custom':

	$query = 'select $object $title $collection $collectionTitle $dsid from <#ri> where';
	$queryParts= array();
	for ($i=1;$i<=6;$i++) {
	  if (isset($_SESSION['fedora_ingest_files']['ROTATOR_IMG'.$i])) {
	    $queryParts[] = 	'($object <dc:identifier> \''.$this->pid.'\' and 
$title <mulgara:is> \''.htmlspecialchars($form_values['description'.$i]).'\' and
$collection <mulgara:is> \''.$this->pid.'\' and
$collectionTitle <mulgara:is> \''.htmlspecialchars($form_values['title'.$i]).'\' and
$dsid <mulgara:is> \'ROTATOR_IMG'.$i.'\' ) ';
	  }
	}
	$query.=join(' or ',$queryParts); 
	break;
	
      case 'collection':
      default:	
	
	$query = 'select $object $title $collection $collectionTitle from <#ri>
where $object <info:fedora/fedora-system:def/relations-external#isMemberOfCollection> <info:fedora/'.$this->pid.'>
and <info:fedora/'.$this->pid.'> <dc:identifier> $collection
and <info:fedora/'.$this->pid.'> <dc:title> $collectionTitle
and $object <fedora-model:state> <info:fedora/fedora-system:def/model#Active>
and $object <dc:title> $title';
	
	break;	
    }
    
    $file = '/tmp/image_rotator.txt';
    @file_put_contents($file, $query); 
    file_move($file); 
    
    $_SESSION['fedora_ingest_files']['IMAGE_ROTATOR_QUERY'] = $file;

  }  
}
