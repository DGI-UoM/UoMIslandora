<?php
// $Id$

class UofMCollectionDisplay
{
  private $pid = NULL;
  private $item = NULL;
  
  function __construct($pid) {
    //drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
    module_load_include('inc', 'fedora_repository', 'api/fedora_item');
    $this->pid = $pid;
    $this->item = new Fedora_Item($this->pid);
  }
  
  
  function showCollection()
  {	
    drupal_set_breadcrumb(array()); //clear breadcrumbs for this page.

    module_load_include('inc', 'fedora_image_blocks', 'image_rotator_block');
   
		
		unset($_SESSION['islandora_solr_page_number']); 
		unset($_SESSION['islandora_solr_limit']);
		unset($_SESSION['islandora_solr_query']);
		unset($_SESSION['islandora_solr_fq']);
		unset($_SESSION['islandora_solr_defThype']);

    
		$dc =  DOMDocument::loadXML($this->item->get_datastream_dissemination('DC'));
    
    $title = $dc->getElementsByTagName('title')->item(0)->nodeValue;
    
    $description = '';
    $descriptionEl = $dc->getElementsByTagName('description');
    if ($descriptionEl->length > 0)
    {
      $description = nl2br($descriptionEl->item(0)->nodeValue);
    }
    
    $query = NULL;
    $datastreams = $this->item->get_datastreams_list_as_array();
    if (isset($datastreams['IMAGE_ROTATOR_QUERY']))
    {
      $query = $this->item->get_datastream_dissemination('IMAGE_ROTATOR_QUERY');
    }
    
    drupal_add_css(drupal_get_path('module','panels').'/plugins/layouts/twocol_stacked/twocol_stacked.css');
    
    
    if ($this->pid == 'uofm:prairieIslamic') {

      $content = '
  <div class="panel-2col-stacked clear-block panel-display">
    <div class="center-wrapper-iac">
      <div class="panel-col-first panel-panel">
        <div class="inside"></div>
      </div>
      <div class="panel-col-last panel-panel" id="image_rotator">
        <div class="inside">
    <div class="panel-pane pane-block pane-fedora-ilives-1" >
      <div class="pane-content">
      '._fedora_image_rotator_block_content($query,'smallthree').'
      </div>
    </div>
        </div>
      </div>
    </div>
  </div><br/><br/>';
  
    } else {
      
      $content = '
  <div class="panel-2col-stacked clear-block panel-display">
    <div class="center-wrapper">
      <div class="panel-col-first panel-panel">
        <div class="inside">
    <div class="panel-pane pane-custom pane-1" id="homepage_intro">
      <div class="pane-content">
      <h2>'.$title.'</h2>'.$description.'
      </div>
    </div>
        </div>
      </div>
      <div class="panel-col-last panel-panel" id="image_rotator">
        <div class="inside">
    <div class="panel-pane pane-block pane-fedora-ilives-1" >
      <div class="pane-content">
      '._fedora_image_rotator_block_content($query,'large').'
      </div>
    </div>
        </div>
      </div>
    </div>
  </div><br/><br/>';
  }

  module_load_include('inc', 'islandora_solr_search', 'IslandoraSolrResults');
  $searchClass = new IslandoraSolrResults();
  
  $startPage=1;
  if (isset($_GET['page']) && intval(trim($_GET['page'])) != 0) {
    $startPage = intval(trim($_GET['page']));
  }
  $filterQuery=  (isset($_GET['fq']))? $_GET['fq'] : '';

  $sort=null;
  $limit=null;
  
  if (isset($_GET['sort'])) {
    $sort = trim(urldecode($_GET['sort']));
  }
  
  if (isset($_GET['limit'])) {
    $limit = intval(trim($_GET['limit']));
    if ($limit == 0) {
      $limit=NULL;
    }    
  }
  $searchResults = $searchClass->searchAndDisplay('related_item_identifier_t:'.preg_replace('/\:/','\\:',$this->pid),$startPage,$filterQuery,NULL,$sort,$limit,FALSE);
   $content .=  $searchResults;
 
  
  $content .= '<br clear="all"/>';
  return $content;
  }
}
