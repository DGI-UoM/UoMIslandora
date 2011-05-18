<?php

// $Id$

module_load_include('inc', 'fedora_repository', 'plugins/ModsFormBuilder');

class BuildingFormBuilder extends ModsFormBuilder 
{
 
 function __construct($pid=null) 
 {
   parent::__construct($pid);
 }
 
 function handleEditMetadataForm(&$form_id, &$form_values, &$soap_client) 
 {
    $dom = new DomDocument("1.0", "UTF-8");
    $dom->formatOutput = TRUE;
    $mods = $this->modsFromForm($form_values,$dom);    
    $dom->appendChild($mods);
    
    $root = null; 
    $this->buildingMods($form_values, $dom, $root);
    
    if ($this->item->modify_datastream_by_value($dom->saveXML(), 'MODS', "MODS Record", 'text/xml') !== NULL) {
        drupal_set_message(t('Successfully updated MODS datastream for object %pid', array('%pid'=>$this->pid)));
    }
    drupal_goto('/fedora/repository/'.$this->pid);
 }  
  
  function handleBuildingForm($form_values)
  {
    module_load_include('inc', 'fedora_repository', 'api/fedora_item');
    module_load_include('inc', 'fedora_repository', 'CollectionPolicy');
    
    $dom = new DomDocument("1.0", "UTF-8");
    $dom->formatOutput = TRUE;
    $pid=$form_values['pid'];
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
    
    //split out metadata that is for the first image object from the building metadata.
    $first_image_form_values = array(); 
    $first_image_form_values['dc_relation'] = $form_values['mods_title']; 
    $first_image_form_values['mods_title'] = $form_values['mods_title']. (isset($form_values['mods_caption'])?' - '. $form_values['mods_caption']:'');
    $first_image_form_values['content_model_pid'] = 'uofm:compoundPrairieBuildingImage';
    $first_image_form_values['relationship'] = 'isPartOf';
    $first_image_form_values['user_id']=$form_values['user_id'];
    
    if (isset($form_values['mods_caption'])) {
      $first_image_form_values['mods_caption']=$form_values['mods_caption'];
      unset($form_values['mods_caption']);
    }
    
    if (isset($form_values['ingest-file-location'])) {
      $first_image_form_values['ingest-file-location']=$form_values['ingest-file-location'];
      unset($form_values['ingest-file-location']);
    }
    
    if (isset($form_values['photographer'])) { 
      $first_image_form_values['mods_people']=array('people'=>array());
      $first_image_form_values['mods_people']['people'][0] = array('name'=>$form_values['photographer'], 'role' => 'pht');
      unset($form_values['photographer']);
    }
    
    if (isset($form_values['mods_dateCaptured'])) { 
      $first_image_form_values['mods_dateCaptured']=$form_values['mods_dateCaptured'];
      unset($form_values['mods_dateCaptured']);
    }
    
    if (isset($form_values['mods_format'])) { 
      $first_image_form_values['mods_format']=$form_values['mods_format'];
      unset($form_values['mods_format']);
    }
    
    if (isset($form_values['mods_cc'])) { 
      $first_image_form_values['mods_cc']=$form_values['mods_cc'];
      unset($form_values['mods_cc']);
    }
    
    if (isset($form_values['mods_rights'])) { 
      $first_image_form_values['mods_rights']=$form_values['mods_rights'];
      unset($form_values['mods_rights']);
    }    
        
    
    //update $_SESSION['fedora_ingest_files'] to only include the thumbnail.
    $image_ingest_files = $_SESSION['fedora_ingest_files'];
    $_SESSION['fedora_ingest_files'] = array(); 
    foreach ($image_ingest_files as $dsid=>$createdFile) { 
      if ($dsid == 'TN') { 
	$_SESSION['fedora_ingest_files'][$dsid]=$createdFile;
      }
    }
        
    
    // Create MODS
    $this->createModsStream($form_values, $dom, $rootElement);
    $this->buildingMods($form_values, $dom, $rootElement);
    
    $this->createCollectionPolicy($form_values, $dom, $rootElement);    
    $this->createWorkflowStream($form_values, $dom, $rootElement);

    if (!empty($form_values['ingest-file-location']) || !empty($_SESSION['fedora_ingest_files'])) {
      $this->createFedoraDataStreams($form_values, $dom, $rootElement);
    }
    $this->createPolicy($collectionPid, &$dom, &$rootElement);

//         header('Content-type: application/xml');
//         echo $dom->saveXML();  exit();

    $objPid = FALSE;
    try {
        $object = Fedora_Item::ingest_from_FOXML($dom);
        //for some reason, ingest_from_FOXML does not generate a JMS message
        //I just modify the workflow DS and it sends a JMS message.  
        $item = new Fedora_Item($object->pid);
        $item->modify_datastream_by_value( $item->get_datastream_dissemination('WORKFLOW'), 'WORKFLOW', "Workflow Record", 'text/xml');
  
        if (!empty($object->pid)) {
	  $objPid=$object->pid;
          drupal_set_message(t("Item !pid created successfully.", array('!pid' => l($object->pid, 'fedora/repository/'. $object->pid))), "status");
         }
    }
    catch (exception $e) {
      drupal_set_message(t('Error ingesting object: !e', array('!e' => $e->getMessage())), 'error');
      watchdog(t("Fedora_Repository"), t("Error ingesting object: !e", array('!e' => $e->getMessage())), NULL, WATCHDOG_ERROR);
      return;
    }    
    
    if ($objPid !== FALSE) { 
      
      if ( ($cp = CollectionPolicy::loadFromCollection($objPid)) !== FALSE) { 
	
	$pid = $cp->getNextPid('ISLANDORACM');
	
	 //restore $_SESSION['fedora_ingest_files'] for the image object.
	$_SESSION['fedora_ingest_files']=$image_ingest_files;
	
	$dom = new DomDocument("1.0", "UTF-8");
	$dom->formatOutput = TRUE;
	$rootElement = $dom->createElement("foxml:digitalObject");
	$rootElement->setAttribute('VERSION', '1.1');
	$rootElement->setAttribute('PID', "$pid");
	$rootElement->setAttribute('xmlns:foxml', "info:fedora/fedora-system:def/foxml#");
	$rootElement->setAttribute('xmlns:xsi', "http://www.w3.org/2001/XMLSchema-instance");
	$rootElement->setAttribute('xsi:schemaLocation', "info:fedora/fedora-system:def/foxml# http://www.fedora.info/definitions/1/0/foxml1-1.xsd");
	$dom->appendChild($rootElement); 
	
	$form_values = $first_image_form_values;
	$form_values['collection_pid'] = $objPid;
	$form_values['pid'] = $pid;
	
	$this->createStandardFedoraStuff($form_values, $dom, $rootElement);
	// Create relationships
	$this->createRelationShips($form_values, $dom, $rootElement);
	  
	// Create MODS
	$this->createModsStream($form_values, $dom, $rootElement);
	$this->buildingMods($form_values, $dom, $rootElement);
	
	$this->createCollectionPolicy($form_values, $dom, $rootElement);    
	$this->createWorkflowStream($form_values, $dom, $rootElement);

	if (!empty($form_values['ingest-file-location'])) {
	  $this->createFedoraDataStreams($form_values, $dom, $rootElement);
	}
	$this->createPolicy($objPid, &$dom, &$rootElement);
	
	try {
	  $object = Fedora_Item::ingest_from_FOXML($dom);
	  //for some reason, ingest_from_FOXML does not generate a JMS message
	  //I just modify the workflow DS and it sends a JMS message.  
	  $item = new Fedora_Item($object->pid);
	  $item->modify_datastream_by_value( $item->get_datastream_dissemination('WORKFLOW'), 'WORKFLOW', "Workflow Record", 'text/xml');
    
	  if (!empty($object->pid)) {
	    $objPid=$object->pid;
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
    }
  }
  
  function buildingMods($form_values, &$dom, &$rootElement) {
    
    if ($rootElement == null) {
      $modsRoot = $dom->getElementsByTagName('mods:mods');
    } else {
      $modsRoot = $rootElement->getElementsByTagName('mods:mods');
    }

    if ($modsRoot->length > 0) { 
      $modsRoot=$modsRoot->item(0);
      
      if (isset($form_values['building_type']) && trim($form_values['building_type']) != '') {
	$subjectEl = $dom->createElement('mods:subject'); 
	$subjectEl->setAttribute('ID','building type');
	$subjectEl->setAttribute('authority','aat');
	$topicEl = $dom->createElement('mods:topic',trim($form_values['building_type']));
	$subjectEl->appendChild($topicEl);
	$modsRoot->appendChild($subjectEl);
      }
      
      if (isset($form_values['building_style']) && trim($form_values['building_style']) != '') {
	$subjectEl = $dom->createElement('mods:subject'); 
	$subjectEl->setAttribute('ID','building style');
	$subjectEl->setAttribute('authority','aat');
	$topicEl = $dom->createElement('mods:topic',trim($form_values['building_style']));
	$subjectEl->appendChild($topicEl);
	$modsRoot->appendChild($subjectEl);
      }      
      
      if (isset($form_values['building_usage']) && trim($form_values['building_usage']) != '') {
	$subjectEl = $dom->createElement('mods:subject'); 
	$subjectEl->setAttribute('ID','building usage');
	$subjectEl->setAttribute('authority','aat');
	$topicEl = $dom->createElement('mods:topic',trim($form_values['building_usage']));
	$subjectEl->appendChild($topicEl);
	$modsRoot->appendChild($subjectEl);
      } 
    }
  }  
}