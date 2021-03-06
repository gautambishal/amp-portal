<?php
/**
 * @file
 * nationalPlanObjective plugin.
 */

$plugin = array(
  'title' => t('National Plan Objective'),
  'description' => t(''),

  'property name' => 'national_plan_objective',
  'property info' => array(
    'type' => 'text',
  ),

  'report field group' => 'columns',
  'report field key' => 'National Planning Objectives Level 1',
  'report filter key' => 'national-planning-objectives-level-1',

  'report filter endpoint' => 'filters/programs',
  'report filter endpoint extra' => 'filters/programs',

  'handler' => 'nationalPlanObjectiveMapper',
);

class nationalPlanObjectiveMapper extends activityMapperSubFilterAutocomplete {
}
